# MNE-CPP 项目面试预设问题整理

## 1. 项目整体介绍

这个项目是基于 MNE-CPP 框架进行扩展的 MEG/EEG 实时采集、在线去噪与可视化系统。MNE-CPP 本身提供跨平台的神经电生理数据处理能力，项目中的重点工作集中在 MNE Scan 实时插件链路中：从采集端或仿真端得到 `RealTimeMultiSampleArray` 数据块，经 `NoiseReduction` 插件进入在线处理线程，使用 Eigen 完成矩阵计算和自适应 tSSS/SSS 类去噪，再把处理后的数据通过 MNE Scan 的 connector 输出给下游显示、写盘或进一步算法模块。

核心实现可以概括为四层：

1. 数据流层：MNE Scan 插件通过 typed connector 连接，输入输出类型主要是 `RealTimeMultiSampleArray`。`NoiseReduction` 插件在 `update()` 中接收 chunk，放入 `CircularBuffer`，后台 `run()` 线程持续 `pop()` 并处理。

2. 算法层：新增/扩展 `AdaptiveTSSS` 实时处理器，基于预计算的 SSS 内部/外部基 `G_in/G_out`，先用带正则的加权最小二乘估计 internal/external Maxwell 系数，再在滑动时间窗内用 CCA/SVD 思路识别 internal 与 external 高相关成分并抑制。代码还加入触发保护、外部噪声子空间跟踪、能量增益保护、NaN/Inf 防护和实时 QC 日志。

3. 系统层：使用 Qt/C++ 组织插件生命周期、信号槽、线程和资源管理。关键对象使用 `QSharedPointer`、`QScopedPointer`、`std::unique_ptr` 等 RAII 手段，数据分块用 `CircularBuffer` 和 `QSemaphore` 实现生产者/消费者缓冲，投影矩阵/补偿矩阵更新时用 `QMutex` 保护共享状态。

4. 可视化层：MNE Scan 的 `RealTime3DWidget` 通过 Qt3D 的 `Qt3DWindow`、`QEntity`、`QTransform`、`QGeometryRenderer`、`QBuffer` 等组件显示实时源定位、连接性、HPI/传感器/脑表面等三维对象；`CustomMesh` 维护顶点、法向、颜色、索引 buffer，`GpuInterpolationItem` 用 GPU buffer/compute command 更新实时插值颜色。

面试中可以把项目定位为：“我不是单纯调用现成库，而是在 MNE-CPP 的实时插件框架里，把 SQUID-MEG 在线去噪算法工程化，包括数据分块调度、Eigen 数值稳定、Qt 插件集成、实时 QC 以及 Qt3D 可视化更新。”

需要提前补充的真实信息：

- 实测 chunk 大小、采样率、通道数、平均延迟、P95/P99 延迟。
- SNR 或 50Hz/工频噪声抑制前后指标。
- SQUID 硬件联调中的真实异常案例，例如失锁、跳变、周期噪声、触发通道污染、线程卡顿。
- 如果简历写了 GPU/FPS/显存监测，需要确认当前代码是否真的实现。现有代码更多是 Qt3D buffer 更新和 OnDemand 渲染，没有看到完整 GPU 占用监测模块。

## 2. 在线分块去噪算法：高频问题

### Q1：你的 SQUID-MEG 在线分块去噪算法原理是什么？

建议回答：

我的算法主线是实时 Adaptive tSSS/SSS。SSS 的核心假设是 MEG 传感器测到的磁场可以用 Maxwell 方程的球谐/多极展开表示，并分解为来自头内的 internal 子空间和外界干扰的 external 子空间。代码里通过离线预计算的 `G_in/G_out` 基矩阵，把每个 chunk 的 MEG 数据 `X` 拟合成：

```text
c = argmin || W([G_in G_out]c - X) ||^2 + lambda ||c||^2
A_in, A_out = split(c)
X_sss = G_in * A_in
```

这里 `W` 用于通道加权，`lambda` 是 ridge 正则，目的是处理 SQUID/MEG 通道量纲差异和基矩阵病态问题。之后 tSSS 在滑动窗口中统计 `A_in` 和 `A_out` 的时间相关性，通过 whitening + SVD 找出 internal 与 external 高相关的成分，把这些成分从 internal 系数中投影掉，从而压制随时间相关的外界干扰。

代码对应：

- `AdaptiveTSSSKernel` 说明了 `matSSSIn/matSSSOut`、ridge、滑动窗口、相关阈值等配置。
- `AdaptiveTSSS::rebuildSpatialSolver()` 构造 `[G_in, G_out, Ufeat]`，做加权、列归一化、正则化和 `LDLT` 求解。
- `AdaptiveTSSS::computeProjectorFromWindow()` 对 internal/external 系数窗口构建协方差，做 `JacobiSVD`，相关性超过阈值的成分进入 removal projector。
- `NoiseReduction::run()` 负责加载 basis、设置 10s temporal memory、0.98 相关阈值、50 chunk 投影更新周期、触发保护和外部噪声子空间配置。

### Q2：为什么需要 tSSS，普通 SSS 不够吗？

建议回答：

SSS 是空间分离，只依赖传感器几何和 Maxwell 基，把空间上更像外界的成分分离出去。但在线 SQUID-MEG 里，外部噪声、环境磁场漂移、硬件状态变化会让 internal/external 在时间上出现相关泄漏，普通 SSS 可能保留一部分动态干扰。tSSS 增加时间维度：在滑动窗口内找 `A_in` 和 `A_out` 的高相关模式，认为这类 internal 成分更可能是外界干扰泄漏，进行抑制。

我这里还加了几类工程保护：

- 触发保护：事件诱发响应附近不能被当成噪声学习。
- warm-up：窗口未填满前不激进更新。
- 投影更新降频：每 50 个 chunk 更新一次，减少 CPU 抖动。
- 能量 clamp：避免病态投影导致输出爆振或过度衰减。
- external noise subspace：对 `A_out` 的主噪声子空间做在线 EWMA 跟踪。

### Q3：在线处理如何保证不把诱发响应去掉？

建议回答：

代码中做了 trigger-aware protection。`NoiseReduction::run()` 里先从 `STI014` 或 `STI 014` 检测触发上升沿，并把 chunk 内触发 offset 传给 `AdaptiveTSSS::setPendingTriggers()`。`AdaptiveTSSS::buildProtectionWeights()` 会在触发前后构造保护窗口，当前配置是 0 到 140ms，加 10ms taper。保护窗口内样本在学习统计时被 hard/soft down-weight，同时输出会走 fallback，避免把诱发响应误学成 external 相关噪声。

还做了 STIM 通道隔离：STIM-like 通道在处理前被置零，处理后从 raw 数据恢复，避免高幅值触发通道通过空间矩阵混进 MEG 通道。

### Q4：性能和效果怎么验证？

现有代码里已经有一组实时 QC 指标：

- 延迟：`QElapsedTimer` 包住 `AdaptiveTSSS::calculate()`，记录 `last_ms/mean_ms/p95_ms`。
- 整体幅值：`rms_pre/rms_post/rms_ratio`。
- 工频噪声：`goertzelAvgPower()` 计算 50Hz 前后能量，得到 `lineRatio`。
- 去噪强度：`relSSS`、`relT`。
- 稳定性：`driftIndex`、`beta_energy`、`hf_ratio_post`、`R_out`、`corr_max`、`k_removed`、`ring_fill/win_samp`。
- 触发保护：`protect_frac`、`carry_in/out`、`nTrig`、`freeze_ms`。

需要你补充真实数字：

```text
采样率: [填写]
通道数: [填写]
chunk 大小: [填写，例如 32/64/128 samples]
平均处理耗时: [填写] ms
P95/P99 处理耗时: [填写] ms
端到端延迟: [填写] ms
50Hz lineRatio: [填写，例如 0.2 表示降到 20%]
RMS/SNR 改善: [填写]
最大噪声压力测试下最差延迟: [填写]
```

面试官如果追问“极端案例”，可以准备：

- 最大噪声/强工频：`lineRatio` 是否仍下降，是否触发 `beta_energy` 或 fallback。
- 触发密集：保护窗口占比升高后，去噪强度下降但诱发响应保真。
- basis mismatch：`G_in/G_out` 行数与 MEG picks 不一致时不处理或回退 raw，避免崩溃。
- STIM 通道异常大：先排除 STIM-like 通道再做空间处理。

### Q5：分块大小、去噪强度、实时性之间的 trade-off 是什么？

建议回答：

chunk 越小，单块等待时间越短，端到端延迟低，但矩阵乘法和信号槽调度的固定开销占比更高，P95 延迟可能抖动。chunk 越大，矩阵运算吞吐更好，SVD/协方差估计更稳定，但端到端延迟会增加，触发保护的时间分辨率也变粗。

tSSS 强度主要由 temporal memory、相关阈值、更新周期、max remove、外部噪声子空间维度等控制。更激进的去噪可以降低外部干扰，但可能损伤脑源信号，尤其是诱发响应和低频慢漂。因此我用较保守配置：10s 记忆窗口、0.98 相关阈值、50 chunk 更新周期、warm-up、trigger protection 和能量 clamp，优先保证在线稳定和不崩采集流。

## 3. 实时数据流与低延迟系统：高频问题

### Q6：实时数据从采集到去噪再到输出的链路是什么？

建议回答：

MNE Scan 通过插件 connector 连接上下游。`NoiseReduction` 是 `AbstractAlgorithm` 插件，`init()` 中创建：

- `PluginInputData<RealTimeMultiSampleArray>`，输入名 `NoiseReductionIn`。
- `PluginOutputData<RealTimeMultiSampleArray>`，输出名 `NoiseReductionOut`。

输入 connector 收到数据后通过 Qt signal 调用 `NoiseReduction::update()`；`update()` 第一次收到数据时初始化 `FiffInfo`、MEG picks、稀疏矩阵和输出对象，然后把每个 chunk push 到 `CircularBuffer`。后台 `run()` 线程从 buffer pop chunk，做 STIM 隔离、滤波、trigger detection、Adaptive tSSS、STIM 恢复，最后 `setValue(matData)` 输出给下游。

### Q7：为什么用 CircularBuffer？它怎么保证线程安全？

建议回答：

采集/仿真线程和算法线程速度不完全一致，不能在数据回调里做重矩阵计算，否则会阻塞采集链路。`CircularBuffer` 用 `QSemaphore` 分别维护 free 和 used 元素，`push()` 先 acquire free，写入后 release used；`pop()` 先 acquire used，读出后 release free。这样形成生产者/消费者模型。

当前 `NoiseReduction` 构造里 buffer 长度是 40；`update()` 里如果 `push()` 失败会 while 等待，所以它提供背压，但极端情况下也可能把上游阻塞。面试可以主动说：“这个设计保证不丢数据，但如果硬实时更重要，可以改成超时丢弃旧 chunk、统计 dropped chunks 或用 lock-free ring buffer。”

### Q8：Qt 信号槽连接方式对低延迟有什么影响？

建议回答：

本项目里有两层连接：

- `NoiseReduction::init()` 把 input connector 的 `notify` 直接连接到 `NoiseReduction::update()`，使用 `Qt::DirectConnection`，使输入通知尽快进入插件。
- 插件间 connector 自动连接时，`PluginConnectorConnection` 对 `RealTimeMultiSampleArray` 使用 `Qt::BlockingQueuedConnection`，注释里说明是因为 simulator 可能从另一个线程发数据，需要跨线程阻塞以保证数据生命周期和顺序。

DirectConnection 延迟最低，但 slot 在 sender 线程执行，重活不能放这里。BlockingQueuedConnection 能跨线程同步，顺序更可控，但有死锁风险：如果 sender/receiver 线程互相等待或 receiver 卡住，就会拖慢整个链路。所以我把真正耗时的算法放到 `run()` 线程，`update()` 只做初始化和入队。

### Q9：Eigen 矩阵计算在哪里可能成为瓶颈？怎么优化？

建议回答：

瓶颈主要在三处：

- 空间拟合：`m_matPinvW * Xw`，大小约为 coeff x channel 乘 channel x samples。
- 投影更新：滑动窗口构建 covariance，做 `SelfAdjointEigenSolver` 和 `JacobiSVD`。
- feature/external covariance 更新：`Crr = Rw * Rw.transpose()`、`Coo += aout_t * aout_t^T`。

优化思路：

- basis 和 pseudo-inverse 只在 picks/kernel 变化时重建，不每个 chunk 重建。
- 用 `noalias()` 避免 Eigen 临时对象，例如 `AinSupp.noalias() = Ain - Ruse * Ain`。
- 对投影更新降频，不每个 chunk 都 SVD。
- 使用 `RowVectorXi` picks 只处理 MEG 通道，排除 STIM/misc。
- 用 ridge、列归一化、特征值裁剪来减少病态矩阵导致的数值放大。

## 4. MNE Scan 插件体系：高频问题

### Q10：MNE Scan 插件要满足什么接口？

建议回答：

插件继承 `AbstractPlugin`，算法类插件继承 `AbstractAlgorithm`。核心接口包括：

- `clone()`
- `init()`
- `unload()`
- `start()`
- `stop()`
- `run()`
- `getType()`
- `getName()`
- `setupWidget()`
- `getBuildInfo()`

`NoiseReduction` 用 `Q_PLUGIN_METADATA` 和 `Q_INTERFACES(AbstractAlgorithm)` 接入 Qt 插件系统，`getType()` 返回 `_IAlgorithm`。它没有叫 `process()` 的统一函数，实际处理入口是 `update()` 接收测量数据，`run()` 中调用 `AdaptiveTSSS::calculate()`。如果面试官说 process/apply，我会解释在这个框架下等价于“connector 输入回调 + 后台线程 calculate”。

### Q11：插件中“在线稳定性处理”具体指什么？

建议回答：

可以从四个层面说：

- 通道稳定：MEG picks 排除 STIM/TRIG 通道，处理前置零、输出前恢复，避免触发信号污染空间投影。
- 数值稳定：ridge 正则、列归一化、`std::isfinite()` 检查、SPD 特征值 floor、投影矩阵特征值裁剪到 `[0,1]`。
- 业务稳定：trigger protection、warm-up、projector update 降频、bad temporal gain 时冻结更新。
- 流水线稳定：basis 未加载或维度不匹配时 `AdaptiveTSSS::calculate()` 返回原始数据/SSS fallback，而不是抛异常中断采集。

### Q12：如果算法计算失败，如何不影响整个采集流？

建议回答：

现有实现里有多处早退 fallback：输入矩阵为空直接返回；basis 为空或维度不匹配时 `m_matPinvW` 为空，`calculate()` 返回 `matData`；SVD/eigen solver 失败时不更新 projector；输出增益异常时 `betaEnergy=0` 并冻结更新。这样即便当前 chunk 无法做 tSSS，采集流仍输出原数据或空间 SSS 结果。

可以补充一个改进点：当前 `NoiseReduction::run()` 主循环没有看到统一 `try/catch` 包住单 chunk 处理。工程上我会再加“单 chunk 异常隔离”：捕获异常、记录 chunk id、输出原始数据、清理 pending trigger，避免异常跨出线程导致插件停止。

## 5. Qt3D + OpenGL 实时渲染：高频问题

### Q13：为什么用 Qt3D，而不是直接 QOpenGLWidget？

建议回答：

这个项目的 3D 内容不是一个简单自绘曲线，而是脑表面、传感器、HPI/digitizer、源定位、连接性网络等多类对象，层级和交互都比较复杂。Qt3D 的 Entity-Component 模型更适合把场景拆成实体和组件：mesh、material、transform、camera、light、picker 都可以独立组合，和 `Data3DTreeModel` 的树形数据结构匹配。

直接用 `QOpenGLWidget` 可以更细粒度控制 VBO/IBO 和渲染循环，但要自己维护 scene graph、拾取、多视角、相机控制、资源生命周期。Qt3D 抽象层更重，但开发效率高，也便于把渲染对象和 UI 控制树打通。

### Q14：Entity-Component 如何映射到 MEG/脑拓扑网格？

建议回答：

`RealTime3DWidget` 持有 `Data3DTreeModel` 和 `View3D`。`View3D` 本身是 `Qt3DWindow`，有 root entity、objects entity、light entity、camera/controller/picker。脑表面、传感器、digitizer、source estimate 等被加入 `Data3DTreeModel`，每个 tree item 对应一个或一组 Qt3D `QEntity`。

几何层由 `CustomMesh` 封装，它继承 `QGeometryRenderer`，内部有 position、normal、color、index 四类 `QBuffer/QAttribute`。动态 HPI 和 digitizer 更新时，通过 `QTransform` 更新坐标变换，例如 `DigitizerTreeItem::setTransform()`、`BemSurfaceTreeItem::applyTransform()`。

### Q15：VBO/IBO 是如何动态更新的？每个 chunk 都上传所有顶点吗？

建议回答要非常诚实：

当前代码没有直接调用 OpenGL 的 `glBufferSubData`，而是通过 Qt3D 的 `QBuffer` 抽象更新 buffer。`CustomMesh::setVertex()`、`setNormals()`、`setColor()`、`setIndex()` 都是构造 `QByteArray` 后调用 `QBuffer::setData()`，这等价于让 Qt3D 后端管理 GPU buffer 更新。

对实时功能，不应该每个 chunk 重建整个脑表面网格。更合理的路径是：

- 静态几何：顶点、法向、索引只初始化或结构变化时上传。
- 动态数据：每个 chunk 只更新颜色/信号 buffer。
- `GpuInterpolationItem::addNewRtData()` 每次把实时 signal vector 写到 `m_pSignalDataBuffer`；插值矩阵只在维度变化时更新，输出颜色 buffer 由 GPU 侧计算。

如果面试官问“是否用了 `glBufferSubData`”，回答：

“在当前 Qt3D 实现里没有直接用 `glBufferSubData`，而是使用 `QBuffer::setData()`。如果进一步优化，我会把每帧更新限定在 signal/color buffer，并评估 Qt3D 的 `updateData()` 或切换到底层 OpenGL buffer 局部更新，避免全量重传。”

### Q16：FPS、CPU/GPU/显存监测怎么实现？

建议回答要区分现状和改进：

现有代码里我看到的是 `View3D` 使用 `QRenderSettings::OnDemand`，并有 screenshot、picker、多视角、灯光控制等功能，但没有看到完整的 FPS/GPU/显存监测模块。因此面试时不能说“已经用 QOpenGLTimerQuery 或 GPUOpen 实现了”。

可以说：

“当前项目里算法耗时有 `QElapsedTimer` 和 P95 日志；渲染侧主要通过 Qt3D 的 OnDemand 降低无效刷新。如果要补完整监控，我会做三层：CPU 用 `QElapsedTimer` 或平台 API 采样，帧率用 frame action/渲染回调统计帧间隔，GPU 时间用 OpenGL timer query 或厂商/系统 API。Qt3D 封装较深，如果需要精确 GPU query，可能要加自定义 frame graph 或退到底层 OpenGL 路径。”

### Q17：监测到 GPU 占用过高时如何动态降级？

建议回答：

可以按对用户感知影响从小到大降级：

- 降低实时数据更新频率，比如 N 个 chunk 更新一次颜色。
- 增大时间平均窗口，减少颜色闪烁和上传频率。
- 关闭 object picking 或降低 picking 精度。
- 降低插值分辨率或只更新 ROI/可见半球。
- 降低脑表面透明叠加、多光源、多视角、连接性边数量。
- 如果 GPU 插值压力高，回退到 CPU 预计算低分辨率颜色，或只上传压缩后的 active vertices。

## 6. SQUID 控制系统与硬件联调：高频问题

### Q18：如何测试 SQUID 系统稳定性？

建议回答框架：

- 静态测试：空房间/phantom 下记录长时间 baseline，看 RMS、PSD、50Hz/100Hz、低频漂移、通道方差。
- 动态测试：人为加入已知干扰或移动条件，看算法是否能稳定抑制外部噪声。
- 事件测试：用 trigger/刺激事件检查诱发响应是否被保护。
- 通道测试：逐通道监控异常幅值、突变、平顶、NaN/Inf、失锁通道。
- 压力测试：最大噪声/最大通道数/最小 chunk 下看 P95/P99 延迟是否仍低于 chunk 周期。

### Q19：遇到过哪些硬件异常，算法如何响应？

你需要补真实经历。可以准备这两个故事模板：

故事 A：触发通道污染 MEG

```text
现象：STI014 触发信号幅值很大，被错误纳入 MEG picks 或参与空间矩阵运算，导致去噪后 MEG 通道出现触发同步尖峰。
排查：看 tSSS basis rows 与 picks 数量不一致，或触发时刻所有 MEG 通道出现同步异常。
解决：根据通道 kind 和名字排除 STI/TRIG；处理前将 STIM-like 通道置零，输出前从 raw 恢复；trigger offsets 单独传给 tSSS 做保护。
结果：[填写指标，例如尖峰消失、P95 延迟无明显变化]
```

故事 B：病态投影导致输出爆振/过度衰减

```text
现象：强噪声或 basis mismatch 下，tSSS 投影后幅值突然变大/变小，表现为 RMS ratio 异常。
排查：查看 corr_max、k_removed、beta_energy、hf_ratio_post、driftIndex 日志。
解决：加入 ridge、列归一化、投影矩阵特征值裁剪、temporal gain clamp、保护窗口 fallback；SVD 失败不更新 projector。
结果：[填写指标，例如异常 chunk 不再拖垮采集流]
```

## 7. 面试代码与设计场景题

### 场景 1：basis 行数和 MEG picks 不一致怎么办？

回答要点：

- basis 必须和实际参与 tSSS 的 MEG 通道顺序一致。
- 排除 STIM/TRIG/misc 后，`G_in/G_out` 行数要么等于 picks 数，要么等于全通道数并按 picks 选行。
- 维度不匹配时不要硬算，应该返回 raw 或 SSS fallback，记录日志。
- 最好在启动时校验并打印 `basis_rows`、`n_meg_picks`、`nchan_total`。

### 场景 2：采集线程很快，算法线程来不及处理怎么办？

回答要点：

- 当前 buffer 提供背压，最多缓存 40 个 matrix chunk。
- 如果强调不丢数据，阻塞上游；如果强调低延迟，允许丢弃旧 chunk 或降低处理频率。
- 可增加 dropped chunk 计数、队列水位监控、动态关闭 tSSS temporal 更新或增加 chunk size。
- 算法上把 SVD 更新降频，basis/pinv 缓存，避免每块重建。

### 场景 3：Qt 信号槽导致死锁怎么排查？

回答要点：

- 检查是否在同线程使用 `BlockingQueuedConnection`。
- 检查 receiver slot 是否等待 sender 线程资源。
- 把重计算从 slot 挪到 worker thread，slot 只入队。
- 用日志打印线程 ID、connector 名称、chunk id、入队出队时间。
- 对 `stop()` 过程注意 `requestInterruption()` 和 `wait()`，避免线程持 mutex 时等待。

### 场景 4：NaN/Inf 输入如何处理？

回答要点：

- 输入层做通道级 sanity check，异常通道本 chunk 置零或保持上一状态，并标记 bad。
- 数值求解前 `std::isfinite()` 清洗权重和参数。
- eigen solver/SVD 失败不更新状态。
- 输出前检查 RMS 和 `isfinite`，异常则 fallback raw。

### 场景 5：每来一个 chunk 都更新 3D 颜色，UI 卡顿怎么办？

回答要点：

- 不重建 entity 和 mesh，只更新 signal/color buffer。
- 对颜色更新节流，比如 30/60 FPS 或每 N 个 chunk 更新一次。
- 关闭 picking、多视角、透明表面或高密连接边。
- 检查是否 `setData()` 导致全量 buffer 重传，必要时改为局部更新或分离静态/动态 buffer。

### 场景 6：`D:/tsss_basis/...` 路径不存在怎么办？

回答要点：

- 当前代码中 basis 和 weights 路径是硬编码，工程化应改为配置项或资源路径。
- 启动时校验文件存在性和矩阵维度；失败时不启用 tSSS，输出 raw/SSS 并提示用户。
- 面试中可以主动把它作为“工程化待改进点”，避免被抓成硬伤。

## 8. C++/Eigen/Qt 基础追问

### Q20：Eigen::Map 和 Matrix 的区别？

建议回答：

`Eigen::MatrixXd` 是拥有内存的矩阵对象，生命周期由它自己管理。`Eigen::Map<MatrixXd>` 是对已有连续内存的非拥有视图，不拷贝数据，适合把外部 buffer、C 数组或 `QByteArray` 中的数据映射成 Eigen 矩阵。

注意点：

- Map 的外部内存必须在 Map 生命周期内有效。
- 要注意行主序/列主序和 stride，否则解释出来的数据顺序会错。
- Map 适合减少实时链路中的拷贝，但不能返回指向临时 buffer 的 Map。

### Q21：Qt 隐式共享机制是什么？

建议回答：

Qt 的很多容器和值类型，例如 `QString`、`QByteArray`、`QVector`，采用 copy-on-write。拷贝对象时先共享同一份数据，只有非 const 修改时才 detach 深拷贝。优点是信号槽或函数传参中按值传递成本较低；风险是如果拿了 `QByteArray::data()` 指针再触发 detach，原指针可能失效。

本项目里 `CustomMesh` 和 `GpuInterpolationItem` 会构造 `QByteArray` 并把 `data()` reinterpret 成 float 数组填充，然后交给 `QBuffer::setData()`。这里要保证填充期间 `QByteArray` 不发生意外 detach，并且字节数、stride、float 对齐都正确。

### Q22：std::move 在 Qt 信号槽中有什么作用？

建议回答：

如果是普通 C++ 函数调用，`std::move` 可以把可移动对象转成右值，避免深拷贝。但 Qt queued signal/slot 需要通过 meta-object 系统传参，参数类型通常要可拷贝或注册 metatype，实际是否移动取决于连接方式和 Qt 版本。对于 `QSharedPointer`，拷贝本身只是引用计数增加，成本较低；随意 `std::move` 反而可能让发送端对象变空，影响后续逻辑。

在这个项目里实时数据更多通过 `QSharedPointer<Measurement>` 和 connector 传递，真正的大矩阵在 `CircularBuffer` push/pop 时仍可能发生 Eigen matrix 拷贝。优化重点应放在减少 matrix 拷贝、复用 buffer、限制每 chunk 重计算，而不是盲目对信号槽参数加 `std::move`。

### Q23：QSharedPointer、QScopedPointer、std::unique_ptr 分别适合什么？

建议回答：

- `QSharedPointer`：跨对象共享所有权，例如插件 connector 和 measurement 对象。
- `QScopedPointer`：当前作用域独占，离开作用域自动释放，例如 `run()` 中的 `FilterOverlapAdd` 和 `AdaptiveTSSS`。
- `std::unique_ptr`：标准 C++ 独占所有权，适合非 QObject 或不依赖 Qt parent-child 的资源，例如 `RealTime3DWidget::alignFiducials()` 中临时 surface set。
- QObject 如果有 parent，也可以交给 Qt parent-child 管理，但要避免和智能指针重复所有权。

## 9. 最应该提前背熟的 8 个点

1. 项目一句话：基于 MNE-CPP/MNE Scan 的 SQUID-MEG 实时采集、在线 Adaptive tSSS 去噪和 Qt3D 可视化系统。
2. 数据链路：输入 connector -> `update()` -> `CircularBuffer` -> `run()` -> `AdaptiveTSSS::calculate()` -> 输出 connector。
3. 算法公式：`[G_in G_out]` ridge WLS 估计系数，internal 重建，滑动窗口 SVD 找 internal/external 高相关模式并抑制。
4. 实时保护：warm-up、投影降频、trigger protection、STIM 通道隔离、gain clamp、fallback raw/SSS。
5. 指标：延迟 `last/mean/p95_ms`，效果 `rms_ratio/lineRatio/relSSS/relT/R_out/corr_max/k_removed`。
6. trade-off：chunk 越小延迟越低但调度开销越大；去噪越强越可能损伤诱发响应，所以用触发保护和保守阈值。
7. Qt3D 诚实点：当前通过 Qt3D `QBuffer::setData()` 更新 buffer，不是直接 `glBufferSubData`；GPU/FPS 监测需要补实现或如实说是改进计划。
8. 真实故事：至少准备 1 个触发通道污染和 1 个数值爆振/线程卡顿/硬件失锁的排查闭环。

## 10. 代码位置速查

- `src/applications/mne_scan/plugins/noisereduction/noisereduction.h`：`NoiseReduction` 插件接口、Qt plugin metadata、输入输出和线程接口。
- `src/applications/mne_scan/plugins/noisereduction/noisereduction.cpp`：connector 初始化、chunk 入队、tSSS 参数、trigger/STIM 保护、实时 QC/PERF 日志。
- `src/libraries/rtprocessing/helpers/adaptivetssskernel.h`：Adaptive tSSS kernel 配置和算法注释。
- `src/libraries/rtprocessing/adaptivetsss.h`：AdaptiveTSSS 对外接口、DebugInfo、保护和降级参数。
- `src/libraries/rtprocessing/adaptivetsss.cpp`：空间 WLS、滑动窗口 SVD、external noise subspace、保护窗口、数值稳定实现。
- `src/libraries/utils/generics/circularbuffer.h`：基于 `QSemaphore` 的线程安全环形缓冲。
- `src/applications/mne_scan/libs/scShared/Plugins/abstractplugin.h` 和 `abstractalgorithm.h`：MNE Scan 插件抽象接口。
- `src/applications/mne_scan/libs/scShared/Management/pluginconnectorconnection.cpp`：插件 connector 的 typed connection 和连接方式。
- `src/applications/mne_scan/libs/scDisp/realtime3dwidget.cpp`：实时 3D 数据入口，source/connectivity/HPI/digitizer 更新。
- `src/libraries/disp3D/engine/view/view3D.cpp`：Qt3D window、root entity、camera、light、picker、OnDemand 渲染。
- `src/libraries/disp3D/engine/model/3dhelpers/custommesh.cpp`：顶点/法向/颜色/索引 QBuffer 更新。
- `src/libraries/disp3D/engine/model/items/common/gpuinterpolationitem.cpp`：GPU 插值 buffer、实时 signal buffer 更新。
