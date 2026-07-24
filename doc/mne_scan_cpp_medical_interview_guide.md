# MNE Scan / MNE-CPP：C++ 医疗软件工程师面试材料（代码审查版）

> 扫描范围：当前工作区中的 `src/applications/mne_scan`、`src/libraries`、顶层 CMake、部署脚本和相关测试。
>
> 表述边界：本文区分“仓库已有机制”“我理解并能讲清楚”“我参与/负责”。没有实际做过的内容，不要把动词改成“负责实现”。当前源码包含本地 `AdaptiveTSSS`/`NoiseReduction` 改动；本文只说明当前代码事实，不据此判断作者归属。
>
> 医疗属性边界：项目明确面向 MEG/EEG 神经电生理采集与处理，文档还称 MNE Scan 在临床环境中使用；但仓库中未检索到 IEC 62304、ISO 14971、法规注册或医疗器械认证证据。因此面试中宜说“医疗/神经电生理软件场景”或“临床相关软件”，不要说“已认证医疗器械软件”。

## 一、可直接使用的项目介绍

### 1. 30 秒项目介绍

我重点研究和参与的是 MNE-CPP 中的 MNE Scan，它是一套基于 C++、Qt 的跨平台 MEG/EEG 实时采集与处理桌面系统。系统把设备或 LSL、FieldTrip Buffer、MNE real-time server 接入的数据统一成 `RealTimeMultiSampleArray`，再通过插件连接器送入滤波、降噪、平均、协方差和源定位等算法，并用二维波形和 Qt3D 场景实时显示。我在面试中会重点讲插件多态、生产者-消费者缓冲、线程退出和实时链路稳定性；具体参与程度应按实际情况表述。

### 2. 1 分钟项目介绍

MNE-CPP 是面向 MEG/EEG 采集、实时处理和离线分析的跨平台 C++ 框架，MNE Scan 是其中的实时桌面应用。程序由 `QApplication` 启动，使用 `QPluginLoader` 加载传感器和算法插件；插件统一继承 `AbstractPlugin`，算法和采集端分别落在 `AbstractAlgorithm`、`AbstractSensor` 接口下，通过 `PluginInputData<T>` 和 `PluginOutputData<T>` 交换强类型测量对象。实时数据通常以 Eigen 矩阵块进入容量受限的 `CircularBuffer`，插件工作线程在 `run()` 中消费并发布下游数据，从而把采集、计算和 UI 显示解耦。项目既有 LSL，也有基于 `QTcpSocket` 的 FieldTrip Buffer 和 MNE real-time server 接入；显示侧既有 Qt Model/View 的多通道波形，也有基于 Entity-Component、camera、geometry buffer 和自定义 framegraph 的 Qt3D 渲染。它很适合体现医疗数据软件关注的实时性、数据格式一致性、线程生命周期、可追溯日志和跨平台部署，但不能把临床使用等同于法规认证。

### 3. 3 分钟技术深挖版

这个项目可以按五层来讲。第一层是应用和插件编排：`mne_scan/main.cpp` 安装统一 Qt 日志处理器、创建 `QApplication`、注册跨线程测量类型并启动 `MainWindow`；`PluginManager::loadPlugins()` 从 `mne_scan_plugins` 目录用 `QPluginLoader` 实例化插件。插件原型通过 `clone()` 生成运行实例，`PluginSceneManager::addPlugin()` 再调用 `init()` 建立输入输出连接器，所以动态库加载和业务实例生命周期是分开的。

第二层是强类型数据管线。`PluginInputData<T>`、`PluginOutputData<T>` 包装统一的 `Measurement` 基类，`PluginConnectorConnection::createConnection()` 只连接类型匹配的接口。以 `NoiseReduction` 为例，`init()` 建立 `RealTimeMultiSampleArray` 输入输出，`update()` 获取 `FiffInfo`、初始化通道和矩阵元数据，并把每个 Eigen 数据块压入 `CircularBuffer<MatrixXd>`；`run()` 在插件线程里消费数据，执行滤波、SPHARA 和当前工作区中的 Adaptive tSSS，再通过 `setValue()` 发出下游通知。这体现了接口隔离和生产者-消费者模型。

第三层是线程与实时性。插件基类直接继承 `QThread`，算法计算放在 `run()`；LSL 插件又展示了更推荐的 worker-object 写法，即把 `LSLAdapterProducer` `moveToThread()`。系统用 `QSemaphore` 有界缓冲实现背压，用 `QMutex`/`QMutexLocker`保护共享参数，用 signal-slot 跨模块传递数据。需要实事求是地指出，代码里大量 `wait(500)` 未验证退出、`while(!push())` 忙等、`BlockingQueuedConnection` 以及缓冲区 `clear()` 删除同步对象，都是实时系统退出崩溃或卡顿的审查重点。

第四层是数据接入和格式。LSL 通过 `resolve_streams()`、`stream_inlet::pull_chunk<float>()` 接收流；FieldTrip 插件用 `QTcpSocket` 发送 `GET_HDR`、`GET_DAT` 等请求；FiffSimulator 使用 4217 命令端口和 4218 数据端口连接 `mne_rt_server`。这些输入最终适配成 `FiffInfo + RealTimeMultiSampleArray`。FIFF 层则由 `FiffStream`、`FiffRawData`、`FiffInfo` 等类负责元数据、通道、坐标变换、投影、补偿和 raw block 的读写。

第五层是可视化和工程化。二维波形由 `QTableView + RtFiffRawViewModel + RtFiffRawViewDelegate` 组织，并可使用 `QOpenGLWidget` viewport。三维部分由 `View3D` 管理 root entity、camera、light 和 picking，`Data3DTreeModel` 把数据树映射到 Qt3D entity；`CustomMesh` 管理 position/normal/color/index `QBuffer` 和 `QAttribute`；`CustomFrameGraph` 组织 surface selector、clear、compute、camera selector、opaque/transparent technique filter、depth/blend/cull 和 render capture。构建以 CMake 为主，同时支持 Qt 5/6、动态/静态插件、Windows/Linux 输出布局；部署脚本分别使用 `windeployqt`、`linuxdeployqt`、`ldd` 和 RPATH。整个项目最适合突出的是：C++ 接口设计、Qt 跨平台、实时线程与缓冲、设备/网络数据适配，以及医疗数据一致性和稳定性意识。

## 二、项目整体定位与证据

### 2.1 一句话定位

MNE Scan 是 MNE-CPP 的实时组件：通过采集插件接入多种 MEG/EEG 系统或网络数据流，通过算法插件在线处理，通过二维/三维视图显示，并可写入 FIFF 文件。

### 2.2 代码与文档证据

| 结论 | 证据 |
|---|---|
| 跨平台 MEG/EEG 实时/离线框架 | `README.md:21` 明确描述 acquisition、processing、MEG/EEG、real-time/offline、cross-platform |
| MNE Scan 是实时组件 | `doc/gh-pages/overview.md:41` |
| 采集/算法插件化 | `src/applications/mne_scan/libs/scShared/Plugins/abstractplugin.h`，`PluginManager::loadPlugins()` |
| 实时矩阵数据 | `src/applications/mne_scan/libs/scMeas/realtimemultisamplearray.cpp`，`RealTimeMultiSampleArray::setValue()` |
| FIFF 医疗/科研数据格式 | `src/libraries/fiff/fiff_stream.h`，`FiffStream::read_meas_info()`、`start_writing_raw()`、`write_raw_buffer()` |
| 临床相关场景 | `doc/gh-pages/overview.md:41` 称 active clinical use；`MainWindow::onGuiModeChanged()` 还提供 Clinical/Research UI 模式 |
| 非认证结论 | 当前扫描未发现 IEC 62304、ISO 14971 或法规认证实现/文档，需要进一步确认 |

### 2.3 技术栈的准确说法

- C++14：`src/CMakeLists.txt:7`。
- Qt 5/Qt 6：各目标以 `find_package(QT NAMES Qt6 Qt5 ...)` 选择版本。
- Widgets、Network、Concurrent、Xml、Qt3DRender：`src/applications/mne_scan/mne_scan/CMakeLists.txt:9-11`。
- Eigen 矩阵：数据块主要是 `Eigen::MatrixXd`，缓冲别名见 `circularbuffer.h:297`。
- FIFF：通道信息、采样率、坏道、投影、补偿、坐标变换、raw/evoked/cov 等类型。
- OpenGL/Qt3D：二维波形可用 `QOpenGLWidget` viewport；三维使用 Qt3D Entity-Component 和 framegraph。
- CMake 为当前主构建系统；qmake 文档属于历史资料，当前根目录没有文档所称的 `mne-cpp.pro`，需要按具体版本进一步确认。

## 三、C++ 面向对象设计能力

### 3.1 技术点、证据、面试表达与追问

#### 技术点 1：抽象接口与运行时多态

**代码证据**

- `src/applications/mne_scan/libs/scShared/Plugins/abstractplugin.h`
  - 类：`AbstractPlugin : public QThread`
  - 函数：纯虚 `clone()`、`init()`、`unload()`、`start()`、`stop()`、`getType()`、`getName()`、`multiInstanceAllowed()`、`setupWidget()`、`getBuildInfo()`、`run()`。
- `src/applications/mne_scan/libs/scShared/Plugins/abstractalgorithm.h`
  - 类：`AbstractAlgorithm : public AbstractPlugin`
  - `Q_DECLARE_INTERFACE(..., "scsharedlib/1.0")`。
- `src/applications/mne_scan/libs/scShared/Plugins/abstractsensor.h`
  - 类：`AbstractSensor`；默认不允许多实例，算法默认允许多实例。

**面试表达**

插件管理器只依赖 `AbstractPlugin`，编排层不需要知道具体设备或算法类。传感器和算法通过更窄的派生接口表达不同语义，运行时再利用 `qobject_cast` 和虚函数完成类型识别与生命周期调用，这是典型的依赖抽象和运行时多态。

**可能追问：为什么析构函数必须是虚函数？**

建议回答：插件实例通常以 `QSharedPointer<AbstractPlugin>` 持有，如果基类析构非虚，通过基类指针释放派生对象会产生未定义行为，派生资源也无法正确清理。这里 `virtual ~AbstractPlugin()` 保证多态销毁，但派生析构仍应先确保线程完全退出。

#### 技术点 2：Prototype/Clone 与动态插件实例分离

**代码证据**

- `src/applications/mne_scan/plugins/noisereduction/noisereduction.cpp:184`：`NoiseReduction::clone()` 返回新实例。
- `src/applications/mne_scan/libs/scShared/Management/pluginscenemanager.cpp:66`：`PluginSceneManager::addPlugin()` 调用 `pPlugin->clone()`，追加实例并调用 `init()`。
- `AbstractAlgorithm::multiInstanceAllowed()` 返回 `true`；`AbstractSensor::multiInstanceAllowed()` 返回 `false`。

**面试表达**

动态库加载得到的是插件原型，用户把插件加入 pipeline 时再通过 `clone()` 创建业务实例。这样既支持算法多实例，也能限制硬件采集插件的单实例约束；比让 UI 直接 `new` 具体类更低耦合。

**可能追问：这是工厂还是原型模式？**

建议回答：`clone()` 和场景管理器的用法更准确地说是原型模式；Qt 的 `QPluginLoader::instance()` 承担动态实例化入口，`PluginInputData<T>::create()` 是轻量静态创建函数。可以说“带有工厂式创建入口”，不必把所有创建行为都拔高为完整抽象工厂。

#### 技术点 3：模板化、强类型输入输出接口

**代码证据**

- `src/applications/mne_scan/libs/scShared/Management/plugininputdata.h`
  - `template<class T> class PluginInputData : public PluginInputConnector`
  - `create()`、`setCallbackMethod()`、`notifyCallbackFunction()`。
- `src/applications/mne_scan/libs/scShared/Management/pluginoutputdata.h/.cpp`
  - `PluginOutputData<T>::measurementData()`、`update()`。
  - 构造时把 `T` 动态转成 `Measurement`，失败则 `qFatal()`。
- `src/applications/mne_scan/libs/scShared/Management/pluginconnectorconnection.cpp`
  - `PluginConnectorConnection::createConnection()` 对 `RealTimeMultiSampleArray`、`RealTimeEvokedSet`、`RealTimeCov`、`RealTimeSourceEstimate` 等成对匹配。

**面试表达**

连接器把具体测量类型放到模板参数里，使算法只声明自己需要的输入和输出类型；对外仍通过统一 `Measurement::SPtr` 信号传递。它兼顾了统一管线和类型约束，减少插件间直接依赖，同时允许连接器层做兼容性检查。

**可能追问：既然有模板，为什么还要 `dynamicCast`？**

建议回答：Qt 信号层使用统一的 `Measurement::SPtr`，运行时连接和插件动态加载无法只靠编译期模板完成，所以边界处仍需运行时检查。更严格的实现可以让连接器暴露类型 ID 并在建连时拒绝不匹配，而不是等数据到达才转换。

#### 技术点 4：插件生命周期协议

**代码证据与语义**

| 函数 | 工程含义 | 具体证据 |
|---|---|---|
| `clone()` | 从原型创建隔离的运行实例 | `NoiseReduction::clone()`；`PluginSceneManager::addPlugin()` |
| `init()` | 建立连接器、测量对象、初始 signal-slot；不应启动长期任务 | `NoiseReduction::init()` |
| `unload()` | 插件从场景移除时保存设置/释放外部资源的扩展点 | 接口注释；`NoiseReduction::unload()` 当前为空，能力存在但未使用 |
| `start()` | 检查配置并启动线程/设备 | `FtBuffer::start()`；`NoiseReduction::start()` 延迟到首块数据再起线程 |
| `stop()` | 停止输入、发中断、等待线程、清缓冲/输出 | `NoiseReduction::stop()`；`LSLAdapter::stop()` |
| `setupWidget()` | 每次选择插件时创建参数页，由 GUI 接管 QWidget 生命周期 | `NoiseReduction::setupWidget()` |
| `update()` | 数据到达回调，做轻量初始化和入队 | `NoiseReduction::update()` |
| `run()` | 工作线程主循环，消费缓冲并计算/发布 | `NoiseReduction::run()` |

**面试表达**

我会把插件生命周期拆成“加载原型、克隆实例、初始化接口、开始运行、停止、卸载”几个阶段。这样 UI、插件管理和算法线程都有清晰边界；真正需要强化的是 `stop()` 的完成条件，必须保证线程已经退出后才能销毁缓冲和插件。

**可能追问：为什么 `NoiseReduction::start()` 不直接启动线程？**

建议回答：算法初始化依赖第一块数据携带的 `FiffInfo` 和 block size，所以 `start()` 只接受启动，`update()` 首次拿到有效数据后才初始化输出、控件并调用 `QThread::start()`。代价是生命周期状态更隐式，工程上最好显式建状态机，例如 `Configured/WaitingMetadata/Running/Stopping/Stopped`。

#### 技术点 5：封装、所有权与 RAII

**代码证据**

- `AbstractPlugin` 把输入输出列表保护在基类中，对外通过访问器暴露。
- 插件广泛使用 `QSharedPointer`、`QPointer`、`QScopedPointer`；例如 `NoiseReduction::run()` 的 `QScopedPointer<FilterOverlapAdd>` 和 `QScopedPointer<AdaptiveTSSS>`。
- `RealTimeMultiSampleArray` 对共享数据使用 `QMutexLocker`。
- `CustomMesh`、`Renderable3DEntity` 使用 Qt parent-child 与 `QPointer` 管理 Qt3D 对象。

**面试表达**

Qt 对象优先利用 parent-child 管理 QObject 生命周期，跨模块共享测量对象用 `QSharedPointer`，线程内独占算法对象用 `QScopedPointer`。锁更适合用 `QMutexLocker` 做作用域释放；当前 `NoiseReduction::run()` 仍有手工 `lock()/unlock()`，异常或提前返回时存在漏解锁风险，是可以明确提出的改进点。

**可能追问：`QPointer` 和 `QSharedPointer` 有什么区别？**

建议回答：`QPointer` 是 QObject 的弱观察指针，QObject 被删除后自动置空，不拥有对象；`QSharedPointer` 通过引用计数共享所有权。已经由 parent 管理的 QWidget/Qt3D node 通常不应再被另一个 shared pointer 重复拥有。

#### 技术点 6：应用入口、配置与日志

**代码证据**

- `src/applications/mne_scan/mne_scan/main.cpp:152-170`
  - `qInstallMessageHandler(ApplicationLogger::customLogWriter)`。
  - 创建 `QApplication`，加载字体，设置组织/应用名供 `QSettings` 使用。
  - `MeasurementTypes::registerTypes()` 注册跨线程 metatype。
- `src/libraries/utils/generics/applicationlogger.cpp:92`
  - `ApplicationLogger::customLogWriter()` 统一处理 Debug/Info/Warning/Critical/Fatal，并用静态 `std::mutex` 串行输出。
- `src/applications/mne_scan/mne_scan/mainwindow.cpp`
  - `saveSettings()`、`loadSettings()`、`startMeasurement()`、`stopMeasurement()`。

**面试表达**

应用启动时先安装全局 Qt 日志处理器，再创建事件循环和注册跨线程类型；主窗口统一管理插件场景、显示和配置。日志目前能按级别着色并避免多线程输出交错，但 `QMessageLogContext` 被忽略、时间戳默认关闭、没有持久文件和轮转，因此只能说“统一日志入口已具备”，不能说“完整审计日志系统”。

**可能追问：MNE Scan 如何解析命令行？**

建议回答：当前 `mne_scan/main.cpp` 没有 `QCommandLineParser`，只使用 `argc/argv` 构造 `QApplication`；仓库其他示例使用了 `QCommandLineParser`，但不能归到 MNE Scan 主程序。如果岗位需要，可扩展 `--plugin-dir`、`--config`、`--log-level` 等参数，并在创建主窗口前完成校验。

### 3.2 最适合重点讲的核心类

1. `AbstractPlugin` / `AbstractAlgorithm` / `AbstractSensor`：讲接口、多态、线程基类和生命周期契约。
2. `PluginManager` / `PluginSceneManager`：讲动态加载、原型克隆、实例编排和启动/停止顺序。
3. `PluginInputData<T>` / `PluginOutputData<T>` / `PluginConnectorConnection`：讲强类型数据管线和模块解耦。
4. `NoiseReduction`：讲从输入回调、元数据初始化、有界缓冲、算法线程到输出通知的完整链路。
5. `RealTimeMultiSampleArray` / `FiffInfo` / `FiffStream`：讲医疗信号块与元数据、格式读写。
6. `View3D` / `Data3DTreeModel` / `CustomFrameGraph` / `CustomMesh`：讲 Qt3D 底层组织。

## 四、Qt / GUI 跨平台开发能力

### 4.1 Qt 技术点总结

| 技术点 | 代码证据 | 面试要点 |
|---|---|---|
| 应用与事件循环 | `mne_scan/main.cpp`：`QApplication app`、`app.exec()` | GUI 主线程只做事件分发和轻量更新 |
| 设置持久化 | `MainWindow::saveSettings()/loadSettings()`，多处 `QSettings` | 组织名/应用名在入口统一设置 |
| 全局日志 | `qInstallMessageHandler()`、`ApplicationLogger::customLogWriter()` | 多线程串行输出、日志分级；持久化仍需扩展 |
| Qt 元对象/插件 | `Q_OBJECT`、`Q_PLUGIN_METADATA`、`Q_INTERFACES`、`Q_DECLARE_INTERFACE` | 动态发现、反射式转换、接口版本 IID |
| 跨线程类型 | `MeasurementTypes::registerTypes()`，LSL 的 `qRegisterMetaType` | queued signal 参数必须注册 |
| 参数界面 | 各插件 `setupWidget()` 和 `FormFiles/*.ui` | QWidget 与算法对象分离，界面可动态替换 |
| signal-slot | connector `notify/update`、GUI controls 到算法 slots | 观察者式通信，降低模块依赖 |
| Model/View | `RtFiffRawView` 的 `QTableView + RtFiffRawViewModel + Delegate`；`Data3DTreeModel` | 数据、渲染和交互职责分离 |
| 2D 加速 | `RtFiffRawView` 给 `QTableView` 设置 `QOpenGLWidget` viewport | 多通道波形绘制减少 QWidget raster 压力 |
| 并发任务 | LSL 扫描用 `QtConcurrent::run` + `QFutureWatcher` | 避免网络发现阻塞 UI |

### 4.2 GUI 与算法模块如何交互

1. 主窗口通过 `PluginGui::selectedPluginChanged` 调用 `MainWindow::updatePluginSetupWidget()`，把当前插件的 `setupWidget()` 放入 central widget。
2. 插件运行后可通过 `AbstractPlugin::pluginControlWidgetsChanged` 发布快速控制面板；`MainWindow::onPluginControlWidgetsChanged()` 把它们加入 `QuickControlView`。
3. 参数控件通过 signal-slot 调算法槽，例如 `NoiseReduction::initPluginControlWidgets()` 将 `ProjectorsView::projSelectionChanged` 连接到 `NoiseReduction::updateProjection()`。
4. 算法输出不是直接操作 UI，而是经 `PluginOutputConnector::notify` 交给 `DisplayManager::show()` 创建的显示 widget。
5. `DisplayManager` 根据输出的测量类型选择 `RealTimeMultiSampleArrayWidget`、`RealTime3DWidget`、`RealTimeCovWidget` 等。这是“算法发布数据，显示层按类型适配”的解耦。

### 4.3 实时数据显示接入

- `RealTimeMultiSampleArray::setValue()` 聚合矩阵块并发出 `Measurement::notify()`。
- `PluginOutputData<T>::update()` 把内部无参 notify 转成带 `Measurement::SPtr` 的 connector notify。
- `DisplayManager::show()` 将 connector 连接到显示 widget 的 `update()`。
- `RealTimeMultiSampleArrayWidget::update()` 首次获取 `FiffInfo` 并创建视图，后续把矩阵块交给 `RtFiffRawView::addData()`。
- `RtFiffRawView` 使用 `RtFiffRawViewModel` 保存显示窗口数据，`RtFiffRawViewDelegate` 生成/绘制通道路径，`QTableView` 负责可见区域和交互。

### 4.4 Qt3D / OpenGL 的准确讲法

不要只说“用了 disp3D”，可以这样展开：

- **View/scene root**：`View3D : Qt3DExtras::Qt3DWindow` 创建 `m_pRootEntity`、三维对象 entity、light entity，并 `setRootEntity()`。
- **Camera**：`View3D` 管理默认 camera 和三个 multiview `QCamera`，用 `QCameraSelector + QViewport` 组成多视口。
- **Entity-Component**：`Renderable3DEntity : Qt3DCore::QEntity` 挂载 `QTransform`、geometry renderer、material 等 component。
- **Geometry/VBO/IBO 抽象**：`CustomMesh : QGeometryRenderer` 创建 position、normal、color、index `QBuffer` 与对应 `QAttribute`，再组装 `QGeometry`。
- **Framegraph**：`CustomFrameGraph : QViewport` 以 `QRenderSurfaceSelector` 为根，包含 `QClearBuffers`、compute dispatch/technique filter、`QCameraSelector`、memory barrier、opaque/transparent technique filter、depth test、cull、blend、sort policy 和 `QRenderCapture`。
- **Model**：`Data3DTreeModel : QStandardItemModel` 同时维护可供控制树展示的数据节点和 `Qt3DCore::QEntity` 根；`RealTime3DWidget` 把 `Control3DView` 设为该 model，并把 model root entity 加到 `View3D` 场景。
- **实时计算**：`RtSensorDataController`、`RtSourceDataController` 把插值和数据 worker 移入独立 `QThread`，避免在 UI/渲染线程做全部插值计算。
- **按需渲染**：`View3D` 设置 `QRenderSettings::OnDemand`，有助于静态场景降低无效重绘；实时数据变化时仍需正确触发更新。

### 4.5 实时可视化的工程难点

1. **吞吐与刷新率不等价**：采样块可能每几毫秒到达，但屏幕通常只需 30/60 FPS；应合并更新或限频，避免每个 chunk 都刷新 UI/GPU buffer。
2. **数据一致性**：当前 `DisplayManager` 使用 `Qt::BlockingQueuedConnection`，原因是测量对象会被下一次 `setValue()` 覆盖。它保证消费者读完再覆盖，却会把 UI 卡顿反向传播到采集链路。
3. **线程归属**：Qt3D entity/component 和 QWidget 应在其所属线程操作；后台 worker 输出普通值对象或不可变快照，再通过 queued signal 回 UI。
4. **大矩阵到颜色 buffer**：应避免重复分配和全量上传，可复用 `QByteArray/QBuffer`、只更新颜色 attribute，并把插值矩阵预计算。
5. **坐标系**：MEG device、head、MRI surface 坐标需通过 `FiffCoordTrans` 一致转换；`RealTime3DWidget` 中存在 `devHeadTrans`、MRI/head alignment 的实际处理。
6. **资源生命周期**：dock/floating 状态变化会影响 OpenGL viewport，`MainWindow::onDockLocationChanged()` 专门调用显示 widget 更新 viewport。

### 4.6 面试中如何说明熟悉 Qt 跨平台

建议表达：我能从 Qt 对象模型、事件循环和线程归属解释这个系统，而不只是会拖 UI。应用层使用 QObject parent-child、`QPointer/QSharedPointer`、`QSettings`、typed signal-slot、metatype 注册和 `QPluginLoader`；显示层使用 Model/View、`QOpenGLWidget` 和 Qt3D framegraph；后台使用 QThread subclass 与 worker-object 两种模式。我也能处理 Windows 的 Qt DLL/plugin 部署和 Linux 的 RPATH、`LD_LIBRARY_PATH`、`ldd`/xcb platform plugin 问题。

### 4.7 可包装到简历的 Qt 描述（按实际经历选择动词）

- 基于 Qt Widgets、signal-slot 和 Model/View 参与/分析 MNE Scan 多通道实时波形界面，实现数据模型、delegate 绘制与参数控制解耦。
- 基于 `QPluginLoader + Q_PLUGIN_METADATA + Q_INTERFACES` 接入/理解采集与算法插件的动态加载及 UI 配置页管理。
- 使用 `QThread`、worker-object、`QtConcurrent/QFutureWatcher` 将流扫描、数据接收和矩阵计算移出 UI 线程。
- 参与/分析 Qt3D Entity-Component 场景、camera、多 viewport、自定义 framegraph 及 geometry buffer 的实时神经数据可视化。
- 排查 Qt signal-slot 连接类型、对象线程归属、OpenGL viewport 和 Qt 版本/插件混用导致的卡顿或崩溃。

## 五、多线程与实时处理能力

### 5.1 项目中的线程模型

| 模型 | 类/文件 | 实际行为 |
|---|---|---|
| QThread 子类作为插件 | `AbstractPlugin : QThread`；`NoiseReduction::run()`、`FtBuffer::run()` | `start()` 后执行重写的 `run()` 循环 |
| 专用 Producer 继承 QThread | `FiffSimulatorProducer`、`TMSIProducer`、`GUSBAmpProducer`、`BrainAMPProducer`、`BabyMEGClient` | 采集/Socket 读取与插件输出线程分离 |
| worker-object + QThread | `LSLAdapterProducer` + `LSLAdapter::m_producerThread` | `moveToThread()`，线程 started 后执行 `readStream()` |
| worker-object + signal command | `FtBuffProducer` + `FtBuffer::m_pProducerThread` | socket、connector、producer 全部移入 producer thread，`workCommand` 启动循环 |
| QtConcurrent | `LSLAdapter::onRefreshAvailableStreams()` | `resolve_streams()` 放入线程池，`QFutureWatcher` 回传结果 |
| 可视化 worker | `RtSensorDataController`、`RtSourceDataController` | 插值和实时颜色/源数据计算分线程 |
| 算法库 worker | `RtAveraging`、`RtConnectivity`、`RtHpi`、`RtInvOp` | 内部 worker thread，析构/停止时 interruption + wait |

关键理解：`QThread` 对象本身仍属于创建它的线程。继承 `QThread` 并不意味着该对象的普通 slot 自动在 `run()` 线程执行；真正的算法代码在 `run()`，而 `NoiseReduction::update()` 因 `Qt::DirectConnection` 在信号发出线程执行。worker-object 模式的线程归属更直观。

### 5.2 实时数据处理链路

以 `FiffSimulator -> NoiseReduction -> Display/WriteToFile` 为例：

```text
RtDataClient(QTcpSocket:4218)
  -> FiffSimulatorProducer 读取 FIFF tag / raw block
  -> FiffSimulator::CircularBuffer<MatrixXd>
  -> FiffSimulator::run()
  -> RealTimeMultiSampleArray::setValue()
  -> PluginOutputData<RealTimeMultiSampleArray>::notify
  -> PluginConnectorConnection
  -> NoiseReduction::update()
  -> NoiseReduction::CircularBuffer<MatrixXd>(容量 40)
  -> NoiseReduction::run(): filter / SPHARA / 当前工作区 AdaptiveTSSS
  -> NoiseReduction output::setValue()
  -> 下游算法、DisplayManager 或 WriteToFile
```

代码证据：

- `RealTimeMultiSampleArray::setValue()`：`src/applications/mne_scan/libs/scMeas/realtimemultisamplearray.cpp:143`。
- `PluginOutputData<T>::update()`：把测量对象包装成 connector 信号。
- `PluginConnectorConnection::createConnection()`：类型匹配后连接 output notify 与 input update。
- `NoiseReduction::update()`：首次初始化 `FiffInfo`、输出和 block size，然后入队。
- `NoiseReduction::run()`：`while(!isInterruptionRequested())` 消费队列并发布。
- `WriteToFile::run()`：消费缓冲，调用 `FiffStream::write_raw_buffer()`。

### 5.3 有界缓冲与背压

`src/libraries/utils/generics/circularbuffer.h` 的 `CircularBuffer<T>` 使用：

- 固定容量数组 `m_pBuffer`；
- `m_pFreeElements` 和 `m_pUsedElements` 两个 `QSemaphore`；
- `push()` 先占用 free semaphore，写完释放 used semaphore；
- `pop()` 先占用 used semaphore，读完释放 free semaphore；
- 默认 `tryAcquire` 超时 1000 ms。

这是真正的有界生产者-消费者队列。容量限制能避免数据无限堆积和内存失控，但当前许多调用者使用 `while(!push()) {}` 重试，等价于“不能丢块、让上游等待”的背压策略。医疗采集场景必须明确策略：不能静默丢数据；如果允许显示层丢帧，也应区分“原始记录链路不可丢”和“UI 预览可只保留最新帧”。

### 5.4 生命周期管理

**全局顺序**

- `MainWindow::startMeasurement()` 先保存 pipeline 配置，再调用 `PluginSceneManager::startPlugins()`；启动失败会 `stopPlugins()` 回滚。
- `PluginSceneManager::startPlugins()` 先启动 sensor，再启动 algorithm。`NoiseReduction` 因等待首块元数据而延迟启动实际线程。
- `MainWindow::stopMeasurement()` 先停插件，再清 display 和 quick controls。
- `PluginSceneManager::stopPlugins()` 先停 sensor，再停非 sensor，意图是先截断新数据，再排空/停止消费者。
- `MainWindow::closeEvent()` 调 `stopMeasurement()`，避免窗口销毁时线程仍运行。

**单插件示例**

- `NoiseReduction::stop()`：`requestInterruption()` -> `wait(500)` -> 清输出。
- `WriteToFile::stop()`：`requestInterruption()` -> 无超时 `wait()`，退出完成性更强，但可能无限等。
- `LSLAdapter::stop()`：设置 producer 停止 -> `m_producerThread.wait()`。
- `FiffSimulatorProducer::~FiffSimulatorProducer()`：`requestInterruption()` -> `wait()`。

### 5.5 线程安全与稳定性风险（代码审查结论）

#### P1：有限等待后继续销毁，无法保证线程已退出

- `NoiseReduction::stop()`、`Averaging::stop()`、`Covariance::stop()`、`RtFwd::stop()` 等大量使用 `wait(500)`，但忽略返回值。
- 如果算法单块计算超过 500 ms，`stop()` 仍返回成功；随后析构 `CircularBuffer`、算法状态或 QObject，可能造成 use-after-free、`QThread: Destroyed while thread is still running`，也可能表现为同步对象销毁时仍有人等待。

**改进**：停止输入，设置 interruption/stop token，唤醒所有阻塞等待；检查 `wait(timeout)` 返回值；超时记录线程状态与栈，禁止销毁资源。生产环境可分“正常等待、升级告警、最终隔离失败插件”，不要直接 `terminate()`。

#### P1：`CircularBuffer::clear()`/析构与等待线程并发不安全

- `CircularBuffer::clear()` 直接 `delete m_pFreeElements` 和 `delete m_pUsedElements` 后重建。
- 析构也直接删除两个 semaphore。
- 没有锁或“已关闭”状态来保证此时不存在 `tryAcquire()`。
- `m_bPause`、读写 index 的生命周期操作也没有统一同步。

这不是代码中已观察到的 `QWaitCondition` 报错，但风险机制相同：同步原语先被销毁，线程仍在等待。仓库中 BabyMEG 声明了多个全局 `QWaitCondition`，当前扫描未找到对应 `wait()/wake()` 调用，是否参与运行需要进一步确认。

**改进**：让 buffer 提供 `close()`，设置 atomic closed flag 并释放 semaphore 唤醒消费者；所有线程 join 后才 `clear()/destroy`。重置队列不要删除正在使用的同步对象。

#### P1：FtBuffer producer 线程退出不完整

- `FtBuffer::stop()` 请求 `m_pProducerThread` 中断后，只轮询约 110 ms，不调用 `wait()`；即使仍运行也会 `m_pFtBuffProducer.clear()` 并重建 producer。
- worker 正在执行 `FtBuffProducer::runMainLoop()` 时清理对象，有潜在悬空访问和跨线程 QObject 删除问题。

**改进**：让 socket loop 的阻塞读取有超时或可取消；发 stop command，在 worker 线程关闭 socket并 emit finished；连接 finished->thread.quit，再 `wait()`，最后销毁 worker。

#### P1/P2：`BlockingQueuedConnection` 的延迟传播与死锁风险

- `PluginConnectorConnection::createConnection()` 对多种测量使用 `Qt::BlockingQueuedConnection`。
- `DisplayManager::show()` 到实时显示 widget 也使用 blocking queued，注释说明测量对象会立即被覆盖，所以必须等消费者读完。
- `pluginconnectorconnection.cpp:125` 的注释称某情况“cannot use BlockingQueuedConnection”，实际 `:128-129` 仍使用该连接，注释与实现不一致。

如果 sender/receiver 属于同一线程，blocking queued 可能死锁；如果 UI 更新慢，采集/算法线程会被同步阻塞。当前实现是以吞吐换取共享测量对象的一致性。

**改进**：建连时断言线程归属；数据改成不可变 `shared_ptr<const Block>`、深拷贝快照或带序号的 buffer，使 UI 使用普通 queued connection；显示只消费最新帧，记录链路消费完整帧。

#### P2：忙等和不可控延迟

- `NoiseReduction::update()`、`WriteToFile::update()`、`FtBuffer::onNewDataAvailable()` 等使用 `while(!buffer->push(...)) {}`。
- 每次 `push()` 最多等待 1 秒，循环又不检查 interruption；停止时可能被入队路径拖住。

**改进**：`push(data, timeout, stopToken)`；超时后按链路策略选择阻塞、告警、丢弃最旧显示帧或触发降级。采集记录链路应报告数据缺口和 sample counter。

#### P2：手工锁覆盖重计算

- `NoiseReduction::run()` 在 pop 后 `m_mutex.lock()`，锁内执行滤波/tSSS 等重计算，最后手工 `unlock()`。
- UI 参数更新也需要同一 mutex，可能长期阻塞；异常将导致漏解锁。

**改进**：用 `QMutexLocker` 只复制配置/共享矩阵快照，然后解锁执行计算；算法对象在线程内独占。参数更新采用版本号或双缓冲。

#### P2：函数内 `static` 状态与多实例冲突

- `NoiseReduction::run()` 中 trigger channel、sample counter、seen IDs、日志 ring 等使用函数内 `static`。
- `AbstractAlgorithm::multiInstanceAllowed()` 默认 `true`，因此多个 NoiseReduction 实例会共享这些状态；停止重启后状态也不自动清零。

**改进**：把运行状态变成实例成员，`start()`/`stop()` 明确 reset；只把真正不可变的常量设为 static。

#### P2：LSL 停止标志不是可靠同步

- `LSLAdapterProducer::m_bIsRunning` 是 `volatile bool`，主线程直接调用 worker 的 `stop()` 写入，worker 线程读取。
- C++ 的 `volatile` 不提供线程间 happens-before，严格说存在数据竞争。

**改进**：`std::atomic_bool`，或 queued stop slot 在 worker 所在线程关闭 inlet；注意如果 worker 长循环占满事件循环，queued stop 可能无法执行，所以还需要可取消的 blocking API/atomic token。

#### P2：测试没有覆盖并发退出

- `test_utils_circularbuffer.cpp` 只覆盖创建、push/pop、容量；没有多线程 clear/destructor/stop 测试。
- `testBufferPushingPopping()` 中 `QVERIFY(resultArray[i] == resultArray[i])` 是自比较，不能验证读出结果。

**改进**：增加高并发 producer/consumer、timeout、close while waiting、重复 start/stop、TSAN/ASan 测试，并修正断言为期望数组比较。

### 5.6 如何避免 UI 线程阻塞

- 网络发现：已有 `QtConcurrent::run(LSLAdapter::scanAvailableLSLStreams)`。
- 数据接收：已有 producer thread 或 worker-object。
- 算法计算：已有插件 `run()` 和内部算法 worker。
- 插值：已有 3D controller worker threads。
- UI 只接收降采样/限频后的不可变快照；不要让 `BlockingQueuedConnection` 把重绘时间传回采集线程。
- 使用 `QElapsedTimer` 记录单块 compute time、queue depth、end-to-end latency；当前 `NoiseReduction::run()` 已对 tSSS 使用 `QElapsedTimer`，可扩展为统一指标。

### 5.7 死锁/退出崩溃的回答模板

> 我先把问题分成“线程没退出、线程在等已销毁对象、锁顺序、blocking queued 同线程”四类。复现时打开线程 ID、插件名、状态和 queue depth 日志，在 Windows 用 Visual Studio Break All/WinDbg 查看所有线程栈，在 Linux 用 gdb `thread apply all bt`，必要时用 TSAN/ASan。代码上先停止生产者，再设置 stop token 并唤醒等待者，检查每个 `wait()` 的返回值，join 完成后才清 buffer 和 QObject；同时审查 `BlockingQueuedConnection` 两端线程归属和锁内是否 emit signal。

### 5.8 多线程追问短答

**问：为什么 UI 线程不能阻塞？**

Qt GUI 和输入事件依赖主事件循环；阻塞会让重绘、交互、queued slot、timer 都停住，还可能使 blocking queued 的生产线程一起停住，最终放大为整条实时链路卡顿。

**问：延迟和不丢数据冲突时怎么选？**

把链路分级：原始采集/存盘链路优先完整性，缓冲高水位必须告警或停止采集；UI 预览链路优先低延迟，可丢旧帧只保留最新；算法链路根据临床含义决定是否允许降采样。不能用一个队列策略覆盖所有消费者。

## 六、网络通信 / 实时数据接入能力

### 6.1 仓库中真实存在的网络/流式证据

#### A. LSLAdapter：liblsl 流发现与 chunk 接入

- 文件：`src/applications/mne_scan/plugins/lsladapter/lsladapter.cpp`
  - `LSLAdapter::scanAvailableLSLStreams()` 调 `lsl::resolve_streams()`。
  - `onRefreshAvailableStreams()` 用 `QtConcurrent::run()` 后台扫描。
  - `prepareFiffInfo()` 将 LSL stream metadata 适配为 `FiffInfo`。
- 文件：`lsladapterproducer.cpp`
  - `readStream()` 创建 `lsl::stream_inlet`、`open_stream()`、`pull_chunk<float>()`。
  - 样本先进入 `m_vBufferedSamples`，达到 `m_iOutputBlockSize` 后整理为 channel x sample 的 `Eigen::MatrixXd`，再 `setValue()` 发布。

准确边界：源码调用 liblsl API，没有手写 UDP/TCP。可补充协议背景“LSL 通常用 UDP 做发现、TCP 做数据传输”，但应明确这是 liblsl 的内部机制，不是本项目自行实现。当前 LSLAdapter 没有明确的独立 Marker 插件逻辑；它会扫描所有流，但 string marker 如何映射到 FIFF/事件未在该实现中体现，需要进一步确认。Marker 在 LSL 中通常可以作为独立事件流，这属于可扩展理解。

#### B. FtBuffer：FieldTrip Buffer TCP 客户端

- 文件：`src/applications/mne_scan/plugins/ftbuffer/ftconnector.cpp`
  - `FtConnector::connect()` 创建 `QTcpSocket` 并 `connectToHost(address, port)`。
  - `getHeader()`/`sendRequest()` 使用 `GET_HDR`；`getData()` 使用 `GET_DAT` 和 `datasel_t`。
  - 解析 `headerdef_t` 的 `nchans/nsamples/nevents/fsample/data_type`，解析 `datadef_t` 并转成 `MatrixXd`。
  - `waitForReadyRead()`/`bytesAvailable()` 处理请求响应。
- 文件：`ftbuffproducer.cpp`
  - `runMainLoop()` 完成连接、header、追赶 buffer、循环取数并 emit `newDataAvailable`。

这是可以直接回答“项目存在 TCP 数据接入”的证据，不需要退化成纯间接能力。

#### C. FiffSimulator / communication：MNE real-time server TCP

- `src/libraries/communication/rtClient/rtcmdclient.h`：`RtCmdClient : QTcpSocket`，注释明确 command port 4217。
- `src/libraries/communication/rtClient/rtdataclient.h`：`RtDataClient : QTcpSocket`，注释明确 data port 4218。
- `RtCmdClient::sendCommandJSON()`：写命令、等待发送/响应。
- `RtDataClient::readRawBuffer()` 等通过 `FiffStream::read_tag()` 读 FIFF tag。
- `src/applications/mne_scan/plugins/fiffsimulator/fiffsimulator.cpp`：`connectCmdClient()` 连接 4217。
- `fiffsimulatorproducer.cpp`：数据端连接 4218，读取 info/raw buffer 并入队。

#### D. 设备插件：数据接入能力

BrainAMP、EEGoSports、GUSBAmp、TMSI、Natus、BabyMEG 等插件包含 producer/driver 或厂商 SDK 适配。这些能证明“异构设备接入和统一数据模型”的架构能力，但不能在没有亲自做过 SDK 联调时说成个人网络开发经验。

### 6.2 如何讲成实时数据接入能力

> 项目的接入层不是把 socket 数据直接交给算法，而是分三步：协议/SDK 层负责连接和读取，producer 线程负责分块与节流，adapter 层把通道数、采样率、通道类型和矩阵统一成 `FiffInfo + RealTimeMultiSampleArray`。后续算法只依赖统一连接器，因此 LSL、FieldTrip Buffer、MNE rt server 或硬件设备可以替换，而无需修改滤波和显示模块。

### 6.3 “你做过网络通信吗”的诚实回答

**若实际调试/修改过网络代码：**

> 做过项目内的实时数据接入和 TCP 客户端调试。比如 FieldTrip Buffer 用 `QTcpSocket` 实现连接、`GET_HDR/GET_DAT` 请求、二进制 header/data 解析和 worker thread 拉流；MNE real-time server 分 4217 命令通道和 4218 FIFF 数据通道。我重点关注半包/超时、断线、线程退出、采样连续性和下游背压，而不只是 socket 能连通。

**若只阅读和理解过：**

> 我不把它包装成我独立完成过完整网络栈，但我系统梳理和调试过项目的实时数据接入。仓库有 FieldTrip Buffer 与 MNE rt server 的 `QTcpSocket` 客户端，也有 liblsl 接入；我能讲清协议读取、线程模型、矩阵分块、断线和背压问题。如果岗位要求从零实现服务端，我会诚实说明这部分实战深度仍需补足。

### 6.4 网络部分不强时的转场方式

不要停在“我只会调用 LSL”。可以转到：

- 网络线程与 UI/算法线程隔离；
- chunk 重组、channel x sample 内存布局；
- 有界缓冲和高水位；
- sample counter、timestamp、丢包/断流检测；
- 断线重连与幂等 start/stop；
- 协议数据适配为统一 FIFF metadata；
- 记录链路与预览链路不同的丢帧策略。

## 七、设计模式与软件架构

### 7.1 插件模式

**位置**：`AbstractPlugin`、`PluginManager::loadPlugins()`、各插件 `Q_PLUGIN_METADATA/Q_INTERFACES`、`mne_scan_plugins` 输出目录。

**为什么**：实现由动态库提供，主程序只依赖稳定接口和 IID，运行时发现 sensor/algorithm。

**解决问题**：设备和算法可独立构建、部署、增加或替换；主程序不需要链接全部具体类型。静态构建时又通过 `Q_IMPORT_PLUGIN`/`Q_INIT_RESOURCE` 兼容。

**面试说法**：这是项目最明确的模式，可以直接说插件架构；同时说明 ABI、Qt 版本、编译器和 Debug/Release 必须一致，否则接口相同也可能加载失败。

### 7.2 原型模式与工厂式创建

**位置**：`AbstractPlugin::clone()`、`NoiseReduction::clone()`、`PluginSceneManager::addPlugin()`；连接器 `PluginInputData<T>::create()`/`PluginOutputData<T>::create()`。

**为什么**：场景从已加载原型复制业务实例；模板连接器通过统一静态入口创建。

**解决问题**：隐藏具体构造、支持算法多实例并统一初始化。

**不过度拔高**：准确说 `clone()` 是原型模式；`create()` 是工厂式 helper，不是复杂抽象工厂。

### 7.3 观察者模式 / signal-slot

**位置**：`Measurement::notify()`、`PluginOutputConnector::notify()`、`PluginInputConnector::update()/notify()`、GUI control signals。

**为什么**：生产者发布事件，不直接调用具体下游；多个算法/显示可订阅。

**解决问题**：降低插件、显示和参数控件的编译期耦合，并支持跨线程排队调用。

**面试说法**：Qt signal-slot 是观察者思想的框架实现，但连接类型是实时系统语义的一部分，不能只说“自动线程安全”。

### 7.4 生产者-消费者

**位置**：`CircularBuffer<T>`；`NoiseReduction::update()/run()`；各 sensor producer + plugin run；`WriteToFile`。

**为什么**：接收端 push，工作线程 pop；QSemaphore 表示空闲/已用槽位。

**解决问题**：解耦瞬时采集速率和计算耗时，并用有界容量控制内存。

**不过度拔高**：当前策略主要是阻塞背压，不代表已经解决延迟和过载；高水位监控和退出安全仍需改进。

### 7.5 MVC / Qt Model-View

**位置**：

- 2D：`RtFiffRawView`、`RtFiffRawViewModel`、`RtFiffRawViewDelegate`、`QTableView`。
- 3D：`Data3DTreeModel : QStandardItemModel`、`Control3DView`、各 TreeItem、`View3D`。

**为什么**：模型保存数据/角色，view 管交互和 viewport，delegate 管具体绘制；3D model 同时驱动控制树和 entity 数据。

**解决问题**：通道选择、坏道、缩放、颜色、阈值与绘制逻辑分离，便于扩展显示类型。

**面试说法**：说“Model/View 思想”比强行说完整 MVC 更准确，因为 controller 职责分散在 widget、signal-slot 和 controller 类中。

### 7.6 策略模式（可扩展理解）

**位置**：不同 `AbstractAlgorithm` 插件可在 pipeline 中选择和组合；`DisplayManager` 按测量类型选择显示 widget。

**为什么**：统一接口下可替换滤波、平均、协方差、源定位等处理策略。

**解决问题**：把算法选择从主程序条件分支移到插件编排。

**不过度拔高**：插件是首要架构，策略是其运行时可替换算法的一种理解；不要声称每个算法内部都实现了 GoF Strategy。

### 7.7 适配器思想

**位置**：`LSLAdapter::prepareFiffInfo()`、`LSLAdapterProducer::readStream()`、`FtConnector`、设备 driver/producer。

**为什么**：把外部协议/SDK 的 stream/header/sample 转成统一 `FiffInfo` 和 `RealTimeMultiSampleArray`。

**解决问题**：算法层不感知 liblsl、FieldTrip 命令或厂商 SDK。

**面试说法**：这是“适配器思想”，类名和数据转换证据明确；不必声称每个类严格按 GoF 双接口结构实现。

### 7.8 分层架构

| 层 | 代码位置 | 职责 |
|---|---|---|
| 应用层 | `src/applications/mne_scan/mne_scan` | 启动、主窗口、pipeline 编辑、生命周期和显示编排 |
| 插件共享层 | `mne_scan/libs/scShared` | 插件接口、manager、connector、connection |
| 采集/算法插件层 | `mne_scan/plugins` | LSL/FieldTrip/设备接入、滤波、平均、协方差、HPI、源定位、存盘 |
| 实时测量层 | `mne_scan/libs/scMeas` | `Measurement` 及各种实时数据对象 |
| 算法库层 | `src/libraries/rtprocessing`、`inverse`、`connectivity`、`fwd` | 可复用数学和信号处理 |
| 数据格式层 | `src/libraries/fiff`、`mne`、`fs` | FIFF、MNE、FreeSurfer 数据与坐标 |
| 显示层 | `src/libraries/disp`、`disp3D`、`mne_scan/libs/scDisp` | 2D Model/View、Qt3D 场景与实时 widget |
| 通信层 | `src/libraries/communication` | MNE real-time TCP command/data client |

这种分层的价值是：设备协议、算法、数据格式和 GUI 可分别测试与演进。现实中的耦合点仍包括 Qt 类型贯穿多层、`Measurement` 对象复用导致 blocking connection，以及插件接口与 QThread 继承绑定。

## 八、Windows / Linux 构建、部署和调试

### 8.1 构建部署技术点

#### 当前构建主线：CMake

- `src/CMakeLists.txt`
  - CMake 最低 3.15，C++14。
  - `BUILD_SHARED_LIBS` 默认 ON，可切静态构建。
  - `BUILD_APPLICATIONS/BUILD_EXAMPLES/BUILD_TESTS` 等选项控制范围。
  - 未指定构建类型时默认 Release。
  - `BINARY_OUTPUT_DIRECTORY` 统一输出；Windows 复制 resources，其他平台创建 symlink。
  - `find_package(QT NAMES Qt6 Qt5)` 支持 Qt 5/6 选择。
- `mne_scan/mne_scan/CMakeLists.txt`
  - `CMAKE_AUTOUIC/AUTOMOC/AUTORCC` 自动处理 `.ui`、MOC、QRC。
  - 链接 Core、Widgets、3DRender、Concurrent、Network、Xml。
  - 静态构建链接具体插件并定义 `STATICBUILD`。
- `mne_scan/plugins/CMakeLists.txt`
  - 插件统一输出到 `apps/mne_scan_plugins`。
  - Windows 将 Debug/Release 输出目录显式归一。
  - 动态库前缀设为空，便于跨平台使用一致插件名。
- `noisereduction/CMakeLists.txt`
  - Windows 使用 FFTW DLL 路径，Linux 使用 `libfftw3.so`，体现平台条件分支。

#### qmake 的准确边界

- `doc/gh-pages/pages/development/buildguide_qmake.md` 描述了历史 qmake 构建方式。
- 当前仓库仅在 `resources/wizards/mnecpp/*/file.pro` 看到模板 `.pro`，没有文档所说的根 `mne-cpp.pro`，也没有主工程 `.pri`。
- 面试应说“了解项目历史 qmake 方式，当前代码以 CMake 为主”，不能说当前主线同时维护完整 qmake 工程，除非另一个分支能确认。

#### Windows 部署

- `tools/deploy.bat:70-80` 遍历 DLL/EXE 调用 `windeployqt`。
- 应用库和插件在 Windows 放入 `out/<Build>/apps` 及 `mne_scan_plugins`。
- 需要保证 Qt major/minor、MSVC/MinGW ABI、x86/x64、Debug/Release 与插件一致。
- 除 Qt DLL 外，还要有 `platforms/qwindows.dll`、Qt3D renderer/plugin、项目自己的 DLL、插件 DLL、FFTW/liblsl/厂商 SDK 运行库和 resources。

#### Linux 部署

- `src/applications/CMakeLists.txt` 设置 `CMAKE_INSTALL_RPATH="${ORIGIN}/../lib"`，scan libs 设置 `${ORIGIN}/`。
- `tools/deploy.bat` Linux 分支复制 Qt `platforms`、`xcbglintegrations`、`.so`，调用 `linuxdeployqt`，并用 `ldd` 检查应用与 `libqxcb.so`。
- `LD_LIBRARY_PATH` 是排查/临时运行手段，发布包更应依赖正确 RPATH 和确定的目录结构。
- Linux 插件加载还受 `.so` 的间接依赖、GL/driver、xcb 系列库和文件权限影响。

#### Debug / Release

- Release 是顶层默认；Debug 会保留符号、关闭部分优化并使用不同 CRT/Qt debug 库（Windows 尤其敏感）。
- Windows 不应把 Qt debug DLL 与 Release 应用/插件混用；MSVC runtime 和 iterator/debug ABI 也要一致。
- 排查崩溃应保留匹配二进制的 PDB（Windows）或未剥离符号/独立 debug symbols（Linux）。

### 8.2 插件加载失败排查案例

**现象**：插件目录中有 DLL/.so，但 MNE Scan 没显示插件或启动即崩溃。

**按层排查**

1. 确认路径：`MainWindow::setupPlugins()` 固定加载 `qApp->applicationDirPath() + "/mne_scan_plugins"`。
2. 确认文件：Windows 代码排除 `.exp/.lib`，其余文件都尝试实例化。
3. 打印加载器错误：当前 `PluginManager::loadPlugins()` 没有记录 `QPluginLoader::errorString()`，应补上文件名、IID 和 errorString。
4. 查直接/间接依赖：Windows 用 Dependencies/`dumpbin /dependents`，Linux 用 `ldd`、`readelf -d`。
5. 核对 ABI：Qt5/Qt6、编译器、架构、Debug/Release、`BUILD_SHARED_LIBS`、插件 IID `scsharedlib/1.0`。
6. 核对 Qt platform/renderer：`qwindows.dll` 或 `libqxcb.so`，以及 Qt3D renderer 和 OpenGL driver。
7. 核对项目 DLL 和第三方库是否在 loader search path/RPATH 中。
8. 在 debugger 里对 `QPluginLoader::instance()` 后的空指针和异常断点，查看首个失败依赖。

**当前额外风险**：动态分支把 `qobject_cast<AbstractPlugin*>(pPlugin)` 的结果直接放入 vector，随后再次转换并解引用；如果目录里出现“能被 Qt 实例化但不是 AbstractPlugin”的对象，可能空指针崩溃。应先校验 cast，再访问 `getType()/getName()`。

### 8.3 Qt 版本混用崩溃排查

回答模板：

> 我先从模块列表确认进程实际加载了哪一套 Qt DLL/.so，而不是只看 PATH 配置。Windows 用 debugger Modules/Process Explorer，Linux 用 `ldd` 和 `/proc/<pid>/maps`；然后核对应用、项目库、插件、platform plugin 和 Qt3D plugin 是否来自同一 Qt kit。插件边界涉及 QObject 元对象和 C++ ABI，Qt major、编译器、架构或 Debug/Release 混用都不能靠复制一个缺失 DLL 解决，应该用同一 toolchain 全量重编并重新部署。

### 8.4 日志如何辅助定位

- `main.cpp` 的 `qInstallMessageHandler` 是全局入口，能捕获 Qt 和业务 `qDebug/qInfo/qWarning/qCritical/qFatal`。
- `ApplicationLogger` 用 `std::mutex` 防止多线程日志交错并按级别着色。
- 插件加载会打印插件名和 `getBuildInfo()`，后者包含 build time/hash，有利于确认二进制版本。
- 当前不足：不记录 `QMessageLogContext`，时间戳关闭，无 thread id、文件持久化、轮转、session/pipeline ID。
- 医疗实时软件更建议结构化字段：timestamp、thread、plugin、patient/session 的匿名标识、sample range、queue depth、latency、event code、error code；敏感信息应脱敏并控制访问。

### 8.5 面试中如何体现 Windows / Linux 经验

> Windows 侧我能处理 CMake kit、MSVC/MinGW 与 Qt ABI、PDB、`windeployqt`、DLL 搜索路径和 platform plugin；Linux 侧能处理 GCC/Clang、RPATH、`.so` 间接依赖、`LD_LIBRARY_PATH`、`ldd/readelf`、xcb/OpenGL 和 gdb core dump。对于动态插件，我会把“文件存在、依赖可解析、ABI 一致、IID 正确、对象能实例化”分层排查，而不是反复复制 DLL。

### 8.6 可写进简历的构建部署表达

- 维护/理解基于 CMake 的 Qt 5/Qt 6 跨平台构建，统一应用、共享库、动态插件和资源输出目录。
- 使用 `windeployqt`、`linuxdeployqt`、RPATH、`ldd`/Dependencies 排查 Qt 与第三方动态库依赖。
- 定位 Qt kit、编译器 ABI、x86/x64、Debug/Release 混用造成的插件加载失败和启动崩溃。
- 完善/分析 `qInstallMessageHandler` 全局日志入口和插件 build hash 输出，提高运行版本可追溯性。

## 九、医疗软件工程师岗位映射

| 岗位要求 | 项目中对应经历/机制 | 代码证据 | 面试表达 |
|---|---|---|---|
| C++ 开发 | C++14、Eigen 矩阵、RAII、智能指针、模板、动态多态 | `src/CMakeLists.txt:7`；`NoiseReduction`；`PluginInputData<T>` | 我能从矩阵数据、所有权、异常/锁安全和性能解释实现，不只会调用 Qt |
| 面向对象设计 | 插件抽象、sensor/algorithm 接口、clone 生命周期 | `abstractplugin.h`；`abstractalgorithm.h`；`pluginscenemanager.cpp` | 主程序依赖抽象，具体插件可运行时替换；clone 是原型模式 |
| Qt 跨平台 GUI | Widgets、signal-slot、Model/View、QSettings、Qt3D | `main.cpp`；`RtFiffRawView`；`View3D`；CMake | 能解释事件循环、线程归属、metatype、view/model/delegate 与部署 |
| 多线程 | QThread subclass、worker-object、QtConcurrent、worker controller | `AbstractPlugin`；`LSLAdapter::init()`；`RtSensorDataController` | 数据接收、算法和插值移出 UI；重点管理 stop/join 和 backpressure |
| 网络通信/实时接入 | LSL、FieldTrip TCP、MNE rt server TCP、设备 producer | `lsladapterproducer.cpp`；`ftconnector.cpp`；`rtcmdclient.h/rtdataclient.h` | 协议层读取后适配成统一 FIFF metadata + Matrix block |
| 设计模式 | Plugin、Prototype、Observer、Producer-Consumer、Model/View、Adapter | scShared、CircularBuffer、Data3DTreeModel、LSLAdapter | 说清为什么、解决什么问题，同时指出实现边界 |
| Windows/Linux 调试 | CMake kit、windeployqt/linuxdeployqt、RPATH、动态库 | `tools/deploy.bat`；应用 CMake | 按路径、依赖、ABI、IID、实例化逐层定位插件问题 |
| 医疗数据处理 | MEG/EEG channel、sampling、stim、bad channel、projection、coordinate transform、FIFF | `FiffInfo`；`RealTimeMultiSampleArray::initFromFiffInfo()`；`FiffStream` | 数据不只是 Matrix，还必须保持通道语义、单位、采样率、事件和坐标一致 |
| 稳定性/可维护性 | 有界队列、停止顺序、统一日志、配置保存、测试框架 | `CircularBuffer`；`startMeasurement()`；`ApplicationLogger`；testframes | 已有机制可复用，也能指出 wait(500)、clear 竞态和 blocking connection 风险 |

## 十、简历项目描述

> 使用规则：只选真实做过的 5-8 条。若主要是二次开发/阅读，把“负责”改成“参与”或“深入分析”；若确实提交并验证过，再使用“实现/优化/解决”。不要写无法给出指标来源的“降低延迟 xx%”。

- 基于 C++14 / Qt 参与 MNE Scan 实时 MEG/EEG 数据采集与处理系统开发，梳理采集、算法、显示和 FIFF 存储的端到端数据链路。
- 基于 `AbstractPlugin/AbstractAlgorithm`、`QPluginLoader` 和 clone 机制接入/分析插件化算法模块，实现强类型输入输出连接与可配置 pipeline。
- 基于 `QThread`、worker-object 和 `CircularBuffer<Eigen::MatrixXd>` 实现/优化数据接收与算法计算解耦，完善 start/stop、背压和资源释放策略。
- 在 `NoiseReduction` 链路中参与/分析实时滤波、SPHARA 与 Adaptive tSSS 分块处理，处理 MEG/STIM 通道选择、trigger 保护和 FIFF 元数据一致性。
- 基于 Qt signal-slot、Model/View 和 `QOpenGLWidget` 参与多通道实时波形显示，支持坏道、缩放、时间窗和 trigger 交互。
- 参与/分析 Qt3D Entity-Component、camera、多 viewport、自定义 framegraph 及 geometry buffer，实现源估计、连接网络或传感器数据的三维显示。
- 接入/分析 LSL、FieldTrip Buffer TCP 和 MNE real-time server 数据流，将异构 stream/header/sample 适配为 `FiffInfo + RealTimeMultiSampleArray`。
- 使用 CMake、`windeployqt`、`linuxdeployqt`、RPATH、`ldd`/Dependencies 排查 Qt 版本、动态库依赖、插件加载和跨平台运行问题。

**不建议直接写的表述**

- “独立设计整个 MNE-CPP 架构”：除非确有事实。
- “实现 TCP/UDP 协议栈”：项目使用 `QTcpSocket` 和 liblsl，不是自研网络栈。
- “通过医疗器械认证”：无代码/文档证据。
- “保证零丢包、零延迟”：没有指标和验证报告。
- “全面使用无锁队列”：实际 `CircularBuffer` 使用 semaphore，不是 lock-free。

## 十一、技术面试问答（28 题）

### Q1：这个项目如何体现 C++ 多态？

`AbstractPlugin` 定义纯虚生命周期接口，`AbstractSensor`、`AbstractAlgorithm` 再细分语义；主程序以基类指针保存插件并调用虚函数。动态库通过 `qobject_cast` 验证 Qt interface，业务实例通过 `clone()` 创建，因此编排层无需依赖 `NoiseReduction` 等具体类型。

### Q2：为什么还要 `Q_INTERFACES` 和 `Q_DECLARE_INTERFACE`？

普通 C++ 虚函数只解决语言级多态；Qt 动态插件还需要把接口和 IID 暴露给元对象系统，`qobject_cast` 才能在插件边界识别实现。`Q_PLUGIN_METADATA` 则提供插件 IID 和 JSON metadata。

### Q3：`clone()` 有什么价值？

加载器得到插件原型，场景通过 `clone()` 创建独立实例。算法默认允许多实例，硬件 sensor 默认单实例；这样可以复用动态加载结果，同时保持每个 pipeline 节点的状态隔离。

### Q4：`init()`、`start()`、`run()` 有什么区别？

`init()` 创建连接器和初始 signal-slot；`start()` 校验运行前条件并启动设备/线程；`run()` 是实际工作线程循环。`NoiseReduction` 因依赖首块 `FiffInfo`，在 `update()` 首次收到数据后才调用 `QThread::start()`。

### Q5：Qt signal-slot 是线程安全的吗？

不能笼统说是。queued connection 能在线程间安全排队传值，但参数类型要注册，传入对象本身仍需满足生命周期和并发访问规则；direct connection 在发射线程执行；blocking queued 还可能死锁或传播延迟。

### Q6：这个项目为什么使用 `BlockingQueuedConnection`？

`DisplayManager` 注释说明 `Measurement` 内部值下一次通知会被覆盖，因此生产者等消费者读完再继续。这保证了当次数据一致性，但会让 UI 卡顿阻塞上游；更好的方案是不可变数据快照或独立显示 ring buffer。

### Q7：继承 `QThread` 和 worker-object 哪个更好？

如果对象本身代表线程任务且只在 `run()` 做循环，继承可用；但 slots 的线程归属容易被误解。worker-object `moveToThread()` 更清楚地表达对象在哪个线程执行，LSLAdapter 就采用该方式；无论哪种，都要设计可取消阻塞和 join。

### Q8：为什么 UI 线程不能做滤波或矩阵分解？

UI 线程负责事件、重绘、timer 和 queued slot。重计算会造成界面无响应，还会使 blocking queued 的采集线程被连带阻塞；因此算法在 plugin `run()` 或 worker thread 中执行，UI 只收限频快照。

### Q9：实时数据如何从采集进入算法？

采集 producer 得到 channel x sample 的 `MatrixXd`，sensor 输出 `RealTimeMultiSampleArray::setValue()`，connector 发 `Measurement::SPtr`；类型匹配的 input 收到后，算法 `update()` 入有界队列，`run()` 消费计算并再次发布。

### Q10：数据缓冲区如何设计？

当前 `CircularBuffer<T>` 是固定容量 FIFO，用 free/used 两个 `QSemaphore` 控制生产和消费。面试要进一步说明容量计算、超时、高低水位、停止唤醒、是否丢旧帧、sample range 和监控指标，而不只说“用了环形队列”。

### Q11：缓冲满了怎么办？

当前多数插件阻塞重试，形成背压。我的设计会区分链路：存盘/关键算法不能静默丢块，超时必须告警或停止；UI 可以只保留最新帧。队列深度持续增长说明处理能力低于输入速率，要优化计算、增大合理缓冲或降级非关键输出。

### Q12：如何排查实时数据卡顿？

先在采集、入队、出队、算法结束、UI render 五个点打 timestamp/sample index，观察 queue depth、block compute p50/p95、UI FPS 和丢块。再判断是网络等待、锁竞争、算法耗时、blocking queued 还是 GPU 上传；当前 `NoiseReduction` 已用 `QElapsedTimer` 测 tSSS 单块耗时，可扩展为全链路指标。

### Q13：插件 stop 时崩溃怎么排查？

检查 stop 是否先截断生产者、是否唤醒阻塞等待、`wait()` 是否成功、QObject 是否在正确线程删除。当前 `wait(500)` 不检查结果和 `CircularBuffer::clear()` 删除 semaphore 是重点；用全线程栈确认谁还在 `tryAcquire()`、socket read 或矩阵计算。

### Q14：如何避免线程死锁？

建立锁顺序，缩小锁范围，锁内不 emit blocking signal，不在持锁时等待另一个线程退出。对 `BlockingQueuedConnection` 断言 sender/receiver 不在同一线程；参数更新用快照，计算不长期持锁。

### Q15：项目中做过网络通信吗？

仓库有明确 TCP：FieldTrip Buffer 的 `QTcpSocket + GET_HDR/GET_DAT`，MNE rt server 的 4217 命令和 4218 FIFF 数据通道；另有 liblsl 流接入。个人表述需按实际贡献选择“实现/调试/深入分析”。

### Q16：LSL 的协议和 Marker 怎么理解？

本项目调用 liblsl 的 `resolve_streams()` 和 `stream_inlet::pull_chunk<float>()`，没有自行实现底层协议。一般可理解为 UDP 发现、TCP 数据传输；Marker 通常是独立事件流，但当前 adapter 未明确实现 string marker 到 FIFF event 的映射，需要进一步确认。

### Q17：FieldTrip Buffer 插件如何取数？

先连接 TCP，发 `GET_HDR` 解析通道数、样本数、事件数、采样率和类型；随后用 sample range 发 `GET_DAT`，解析 data definition 和 payload，转为 Eigen 矩阵，再由 producer signal 交给插件 ring buffer。

### Q18：什么是 FIFF？项目里保存了什么？

FIFF 是 MNE 生态使用的标签/块式神经电生理数据格式。项目通过 `FiffInfo` 表达通道名、类型、单位、采样率、坏道、投影/补偿、digitizer 和坐标变换；`FiffStream` 负责 tag、measurement info、raw block、cov 等读写。

### Q19：为什么不能只传一个 Eigen Matrix？

矩阵只包含数值，不包含通道顺序、单位、采样率、坏道、sensor geometry、event 和坐标系。医疗/神经信号处理如果元数据错位，算法仍可能产出“看似正常”的错误结果，所以 `FiffInfo` 与 sample block 必须有版本和一致性校验。

### Q20：NoiseReduction 做了什么？

代码包含 SSP/compensator/SPHARA/Filter 相关矩阵与控制，当前工作区还在 `run()` 中构造 `AdaptiveTSSS`，按 MEG picks 处理并保护/恢复 STIM-like channel。面试只讲自己真正实现或验证的部分；若只读过代码，应说“理解该链路”。

### Q21：当前 Adaptive tSSS 接入有哪些工程风险？

基础矩阵和权重路径硬编码为 `D:/tsss_basis/...`，不跨平台；`run()` 中多项状态是 static，多实例/重启会串状态；重计算位于 mutex 临界区；basis row 与 pick 顺序需要严格校验。应改为配置/资源路径、实例状态、维度与有限值校验，并建立离线金标准和性能门限。

### Q22：Qt3D 场景由哪些底层对象组成？

`View3D` 管 root entity、camera、light、picker；Renderable entity 挂 transform、geometry renderer 和 material；`CustomMesh` 用 position/normal/color/index buffer 与 attribute 建 geometry；`CustomFrameGraph` 决定 surface、clear、camera、compute、opaque/transparent pass、depth/blend/cull 和 capture。

### Q23：Qt3D 实时更新怎么避免卡顿？

采样速率与渲染帧率解耦，后台预计算插值，UI 只取最新帧；复用 geometry 和 GPU buffer，尽量只更新 color/instance 数据；限频并监控 CPU 插值、主线程、render thread 和 GPU upload。项目已有 worker controller 和 OnDemand render policy 可作为基础。

### Q24：Windows 插件加载失败怎么查？

确认 `apps/mne_scan_plugins` 路径，再查 Qt/项目/第三方 DLL 依赖、架构、MSVC/MinGW、Debug/Release、IID 和 metadata；用 Dependencies 或 debugger Modules 看实际加载库，并输出 `QPluginLoader::errorString()`。不要只复制报错里最后一个 DLL。

### Q25：Linux 下程序能编译但不能运行怎么查？

用 `ldd` 查 not found，用 `readelf -d` 看 RPATH/RUNPATH，确认 `${ORIGIN}/../lib` 布局；再查 `libqxcb.so`、xcb 间接依赖、Qt3D renderer、OpenGL driver和权限。临时 `LD_LIBRARY_PATH` 能定位问题，但发布包应修 RPATH/部署内容。

### Q26：如何定位 C++ 崩溃和内存问题？

先保留符号和 core/PDB，按崩溃线程栈、异常/信号、最近生命周期事件定位；Windows 用 VS/WinDbg，Linux 用 gdb/core。再用 ASan/UBSan/TSan、Qt fatal warnings 和最小化 pipeline 验证；插件边界尤其查 ABI、对象所有权、线程退出与静态状态。

### Q27：如何避免内存泄漏？

普通 C++ 独占资源用 `unique_ptr/QScopedPointer`，共享不可变测量用 `QSharedPointer`；QObject 优先 parent-child，观察 QObject 用 `QPointer`。避免 parent 已拥有对象又交给 shared pointer；worker 必须在线程停止后按正确线程语义删除，不能依赖已停止事件循环里的 `deleteLater()` 一定执行。

### Q28：如何保证医疗软件稳定性？你最大的技术难点是什么？

建议回答：最大的难点不是某个矩阵公式，而是持续数据流下同时保证数值正确、时序连续和可退出。我的方法是定义通道/采样/event 元数据不变量，按 sample range 做完整性检查；用有界队列、超时、状态机、结构化日志和指标控制过载；用离线金标准、回放、长稳、重复启停、异常断流和 sanitizer 测试覆盖。还要明确：项目当前未体现完整医疗法规流程，若产品化需补风险管理、需求追踪、验证记录、权限与隐私控制。

## 十二、最推荐强调的 5 个技术亮点

### 1. 强类型插件化实时数据管线

`AbstractPlugin + clone + PluginInputData<T>/PluginOutputData<T> + PluginSceneManager` 同时体现 C++ 多态、模板、生命周期、动态库和低耦合，是岗位要求覆盖面最广的亮点。

### 2. 多线程生产者-消费者与稳定性审查能力

不仅能说 QThread 和环形队列，还能指出 semaphore、背压、blocking queued、stop/join、同步对象析构和 UI 延迟传播。这最贴近医疗实时采集对稳定性的要求。

### 3. MEG/EEG + FIFF 的领域数据一致性

能讲通道类型/顺序、单位、采样率、STIM/event、坏道、投影/补偿、坐标变换和 raw block，说明理解“医疗数据不是普通矩阵”。

### 4. 真实的异构实时接入

LSL、FieldTrip TCP、MNE rt server 双 TCP 通道和设备 producer 都有源码证据，能够把网络通信、硬件接入、adapter 和 buffer 组织成一条完整链路。

### 5. Qt 跨平台 GUI 与 Qt3D 底层组织

从 Widgets、signal-slot、Model/View、OpenGL viewport 一直讲到 entity/component、camera、geometry buffer 和 framegraph，再结合 windeployqt/RPATH，能证明 Qt 能力不止停留在界面控件。

## 十三、面试使用建议

1. 开场用 30 秒版；技术面转到 `NoiseReduction` 数据链路和 `PluginConnectorConnection`。
2. 主动承认两个边界：MNE Scan 主程序没有 `QCommandLineParser`；项目是临床相关 MEG/EEG 软件，但没有仓库内认证证据。
3. 准备画一张 producer -> typed connector -> bounded buffer -> algorithm -> display/file 的图。
4. 准备一个真实排障故事，至少包含现象、假设、线程栈/日志证据、修改、回归验证；没有做过就不要虚构，可改讲代码审查发现和建议方案。
5. 对个人贡献使用分级动词：`负责实现` > `参与开发/优化` > `调试排查` > `深入阅读并能讲清楚`。让技术深度来自证据，而不是放大职责。

## 十四、源码位置速查

| 主题 | 文件/函数 |
|---|---|
| 入口/日志 | `mne_scan/mne_scan/main.cpp::main()`；`utils/generics/applicationlogger.cpp::customLogWriter()` |
| 主生命周期 | `mainwindow.cpp::startMeasurement()/stopMeasurement()/closeEvent()` |
| 插件接口 | `scShared/Plugins/abstractplugin.h`、`abstractalgorithm.h`、`abstractsensor.h` |
| 动态加载 | `scShared/Management/pluginmanager.cpp::loadPlugins()` |
| clone/场景 | `pluginscenemanager.cpp::addPlugin()/startPlugins()/stopPlugins()` |
| 类型连接 | `plugininputdata.h/.cpp`、`pluginoutputdata.h/.cpp`、`pluginconnectorconnection.cpp::createConnection()` |
| 实时测量 | `scMeas/realtimemultisamplearray.cpp::initFromFiffInfo()/setValue()` |
| 算法实例 | `plugins/noisereduction/noisereduction.cpp::init()/update()/run()/stop()` |
| 有界队列 | `utils/generics/circularbuffer.h::push()/pop()/clear()` |
| LSL | `lsladapter.cpp::scanAvailableLSLStreams()/prepareFiffInfo()`；`lsladapterproducer.cpp::readStream()` |
| FieldTrip TCP | `ftbuffer/ftconnector.cpp::connect()/getHeader()/getData()/sendRequest()` |
| MNE rt TCP | `communication/rtClient/rtcmdclient.*`、`rtdataclient.*`、`fiffsimulatorproducer.cpp` |
| FIFF | `fiff/fiff_stream.h/.cpp`、`fiff_info.h`、`fiff_raw_data.h` |
| 2D 波形 | `scDisp/realtimemultisamplearraywidget.cpp`；`disp/viewers/rtfiffrawview.cpp`及 model/delegate |
| Qt3D view | `disp3D/engine/view/view3D.*`、`customframegraph.*` |
| Qt3D model/entity | `data3Dtreemodel.*`、`renderable3Dentity.*`、`custommesh.*` |
| CMake/部署 | `src/CMakeLists.txt`、`mne_scan/**/CMakeLists.txt`、`tools/build_project.bat`、`tools/deploy.bat` |

---

**复核说明**：本文是基于当前源码的静态审查材料，未执行设备联调、网络互通、长稳或完整构建测试。涉及个人职责、性能指标、临床验证和法规状态的内容，必须由实际项目记录进一步确认。
