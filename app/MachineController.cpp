#include "MachineController.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
const sf::Color kBackgroundColor(8, 10, 18);

sim::Address toBlockAddress(std::size_t blockIndex) {
    return static_cast<sim::Address>(blockIndex) * sim::RAM::kCacheLineSizeInBytes;
}

void appendSampledPath(view::rails::RailPath& destination,
                       const view::rails::RailPath& source,
                       float step = 12.0f) {
    if (source.isEmpty()) {
        return;
    }

    const float length = source.getLength();
    sf::Vector2f previousPoint = source.getStartPoint();
    for (float distance = step; distance < length; distance += step) {
        const sf::Vector2f point = source.samplePoint(distance);
        destination.appendStraight(previousPoint, point);
        previousPoint = point;
    }
    destination.appendStraight(previousPoint, source.getEndPoint());
}

view::rails::RailPath buildRegisterLoadPath(const view::CacheView& cacheView,
                                            std::size_t slotIndex,
                                            std::size_t cellIndex,
                                            const view::rails::RailPath& cacheToAluPath,
                                            float thickness) {
    view::rails::RailPath path({thickness, sf::Color(247, 214, 92, 210)});
    const sf::Vector2f cellCenter = cacheView.getCellCenter(slotIndex, cellIndex);
    const view::rails::RailPath& outputPath = cacheView.getOutputPath(slotIndex);
    if (!outputPath.isEmpty()) {
        const sf::Vector2f outputStart = outputPath.getStartPoint();
        if (cellCenter.x > outputStart.x + 0.5f) {
            path.appendStraight(cellCenter, {outputStart.x, cellCenter.y});
        }
        appendSampledPath(path, outputPath);
    }
    appendSampledPath(path, cacheToAluPath);
    return path;
}
} // namespace

MachineController::StructArray::StructArray(StructLayout layoutValue, std::size_t count)
    : layout(std::move(layoutValue)),
      instances(count, StructValues(layout.fieldNames.size(), 0.0f)) {
}

void MachineController::StructArray::set(std::size_t structIndex, std::size_t fieldIndex, float value) {
    if (structIndex >= instances.size()) {
        throw std::out_of_range("Struct index is out of range");
    }
    if (fieldIndex >= layout.fieldNames.size()) {
        throw std::out_of_range("Field index is out of range");
    }

    instances[structIndex][fieldIndex] = value;
}

void MachineController::StructArray::set(std::size_t structIndex, std::string_view fieldName, float value) {
    set(structIndex, findFieldIndex(fieldName), value);
}

std::size_t MachineController::StructArray::findFieldIndex(std::string_view fieldName) const {
    for (std::size_t index = 0; index < layout.fieldNames.size(); ++index) {
        if (layout.fieldNames[index] == fieldName) {
            return index;
        }
    }

    throw std::invalid_argument("Field name is not present in struct layout");
}

MachineController::MachineController()
    : sceneView(kInitialCameraPosition, {1600.0f, 900.0f}),
      camera(sceneView, 650.0f, 0.1f),
      scene(),
      simulation(64 * 6),
      readAnimation(scene.getFont()),
      registerLoadAnimation(scene.getFont()),
      rng(std::random_device{}()) {
    camera.reset(kInitialCameraPosition, kInitialZoom);
    zmmLabels.fill("");
}

view::RamView& MachineController::createRam(const std::string& id,
                                            std::size_t sizeInBytes,
                                            sf::Vector2f position) {
    if (auto it = ramViews.find(id); it != ramViews.end()) {
        return *it->second;
    }

    auto ramView = std::make_unique<view::RamView>(sizeInBytes, scene.getFont(), position);
    view::RamView* ramPtr = ramView.get();
    ramViews.emplace(id, std::move(ramView));
    topLevelBlocks.push_back(ramPtr);
    rebuildComponentRegistry();
    return *ramPtr;
}

view::CpuView& MachineController::createCpu(const std::string& id, sf::Vector2f position) {
    if (auto it = cpuViews.find(id); it != cpuViews.end()) {
        return *it->second;
    }

    auto cpuView = std::make_unique<view::CpuView>(scene.getFont(), position);
    view::CpuView* cpuPtr = cpuView.get();
    cpuViews.emplace(id, std::move(cpuView));
    topLevelBlocks.push_back(cpuPtr);
    rebuildComponentRegistry();
    return *cpuPtr;
}

view::BusView& MachineController::connect(const std::string& id,
                                          std::string fromComponentId,
                                          std::string fromPortId,
                                          std::string toComponentId,
                                          std::string toPortId) {
    for (Connection& connection : connections) {
        if (connection.id == id) {
            connection.fromComponentId = std::move(fromComponentId);
            connection.fromPortId = std::move(fromPortId);
            connection.toComponentId = std::move(toComponentId);
            connection.toPortId = std::move(toPortId);
            refreshConnections();
            return connection.bus;
        }
    }

    Connection connection;
    connection.id = id;
    connection.fromComponentId = std::move(fromComponentId);
    connection.fromPortId = std::move(fromPortId);
    connection.toComponentId = std::move(toComponentId);
    connection.toPortId = std::move(toPortId);
    connections.push_back(std::move(connection));
    refreshConnections();
    return connections.back().bus;
}

view::BlockView* MachineController::findComponent(std::string_view id) {
    if (auto it = componentViewsById.find(std::string(id)); it != componentViewsById.end()) {
        return it->second;
    }

    return nullptr;
}

const view::BlockView* MachineController::findComponent(std::string_view id) const {
    if (auto it = componentViewsById.find(std::string(id)); it != componentViewsById.end()) {
        return it->second;
    }

    return nullptr;
}

view::BusView* MachineController::findConnection(std::string_view id) {
    for (Connection& connection : connections) {
        if (connection.id == id) {
            return &connection.bus;
        }
    }

    return nullptr;
}

const view::BusView* MachineController::findConnection(std::string_view id) const {
    for (const Connection& connection : connections) {
        if (connection.id == id) {
            return &connection.bus;
        }
    }

    return nullptr;
}

MachineController::StructArray MachineController::createStructArray(StructLayout layout, std::size_t count) {
    return StructArray(std::move(layout), count);
}

void MachineController::writeStructArray(std::string_view ramId,
                                         std::initializer_list<std::string> fieldNames,
                                         std::size_t count,
                                         LabelMode labelMode,
                                         sim::Address startAddress) {
    writeStructArray(
        ramId, createStructArray({std::vector<std::string>(fieldNames)}, count), labelMode, startAddress);
}

void MachineController::writeStructArray(std::string_view ramId,
                                         const StructLayout& layout,
                                         const std::vector<StructValues>& instances,
                                         LabelMode labelMode,
                                         sim::Address startAddress) {
    view::RamView* ramView = nullptr;
    if (auto it = ramViews.find(std::string(ramId)); it != ramViews.end()) {
        ramView = it->second.get();
    }
    if (!ramView) {
        throw std::invalid_argument("RAM component not found for struct initialization");
    }

    if (layout.fieldNames.empty()) {
        throw std::invalid_argument("Struct layout must contain at least one field");
    }

    simulation.getRam().clear();

    const std::size_t fieldCount = layout.fieldNames.size();
    const std::size_t bytesPerStruct = fieldCount * sim::RAM::kFloatSizeInBytes;
    sim::Address currentAddress = startAddress;

    for (std::size_t structIndex = 0; structIndex < instances.size(); ++structIndex) {
        const StructValues& values = instances[structIndex];
        if (values.size() != fieldCount) {
            throw std::invalid_argument("Struct instance size does not match layout field count");
        }

        std::vector<std::string> labels;
        labels.reserve(fieldCount);
        for (std::size_t fieldIndex = 0; fieldIndex < layout.fieldNames.size(); ++fieldIndex) {
            if (labelMode == LabelMode::SequentialIndices) {
                labels.push_back(std::to_string(structIndex * fieldCount + fieldIndex));
            } else {
                labels.push_back(layout.fieldNames[fieldIndex] + "[" + std::to_string(structIndex) + "]");
            }
        }

        simulation.getRam().writeFloatArray(currentAddress, values);
        simulation.getRam().setFloatLabels(currentAddress, labels);
        currentAddress += static_cast<sim::Address>(bytesPerStruct);
    }
}

void MachineController::writeStructArray(std::string_view ramId,
                                         const StructArray& array,
                                         LabelMode labelMode,
                                         sim::Address startAddress) {
    writeStructArray(ramId, array.getLayout(), array.getInstances(), labelMode, startAddress);
}

void MachineController::refreshVisualState() {
    syncIdleState();
    refreshConnections();
}

Controls::EventActions MachineController::handleEvents(sf::RenderWindow& window) {
    return Controls::handleEvents(window, camera, sceneView, kInitialCameraPosition, kInitialZoom);
}

void MachineController::update(sf::RenderWindow& window, float deltaSeconds) {
    sf::Vector2f movement = Controls::readMovement();
    if (movement != sf::Vector2f(0.0f, 0.0f)) {
        const float lengthSquared = movement.x * movement.x + movement.y * movement.y;
        if (lengthSquared > 0.0f) {
            const float invLength = 1.0f / std::sqrt(lengthSquared);
            movement *= invLength;
        }

        camera.move(movement * camera.getSpeed() * deltaSeconds);
    }

    const sf::Vector2f mouseWorldPosition = camera.screenToWorld(sf::Mouse::getPosition(window));
    const bool leftMousePressed = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);

    view::BlockView* hoveredBlock = nullptr;
    for (auto it = topLevelBlocks.rbegin(); it != topLevelBlocks.rend(); ++it) {
        if ((*it)->isInDragHandle(mouseWorldPosition)) {
            hoveredBlock = *it;
            break;
        }
    }

    if (leftMousePressed && !previousLeftMousePressed && hoveredBlock) {
        dragTarget.kind = DragTargetKind::Block;
        dragTarget.block = hoveredBlock;
        dragOffset = mouseWorldPosition - hoveredBlock->getPosition();
    }

    if (!leftMousePressed) {
        dragTarget = {};
    } else if (dragTarget.kind == DragTargetKind::Block && dragTarget.block != nullptr) {
        dragTarget.block->setPosition(mouseWorldPosition - dragOffset);
    }

    for (view::BlockView* block : topLevelBlocks) {
        const bool hovered = block == hoveredBlock;
        const bool dragging = dragTarget.kind == DragTargetKind::Block && dragTarget.block == block;
        block->setDragState(hovered || dragging, dragging);
    }

    previousLeftMousePressed = leftMousePressed;
    refreshConnections();

    simulationTickAccumulator += deltaSeconds * animationTiming.simulationTicksPerSecond;
    while (simulationTickAccumulator >= 1.0f) {
        simulation.advance(1);
        simulationTickAccumulator -= 1.0f;
    }

    const sim::MemoryTransaction* activeTransaction = findActiveTransaction();
    if (!activeTransaction && !registerLoadState.active) {
        const auto& transactions = simulation.getTransactions();
        if (!transactions.empty()) {
            const sim::MemoryTransaction& latestTransaction = transactions.back();
            if (latestTransaction.isCompleted(simulation.getCurrentTick()) &&
                latestTransaction.getId() != lastRegisterLoadTransactionId) {
                startRegisterLoad(latestTransaction);
            }
        }
    }

    if (!activeTransaction && !registerLoadState.active) {
        const sim::Address address = chooseRandomAnimatedAddress();
        const std::size_t lineIndex = static_cast<std::size_t>(address / sim::RAM::kCacheLineSizeInBytes);
        const std::size_t targetCacheSlotIndex = simulation.getCache().getTargetSlotIndex(address);

        view::RamView* demoRam = getDemoRam();
        view::CacheView* demoPrimaryCache = getDemoPrimaryCache();
        const view::BusView* demoConnection = findConnection(kDemoConnectionId);
        if (demoRam && demoPrimaryCache && demoConnection) {
            const view::RamView::ReadPath previewReadPath = demoRam->getReadPath(lineIndex);
            demoRam->sync(simulation.getRam());
            if (view::CpuView* demoCpu = getDemoCpu()) {
                demoCpu->syncPrimaryCache(simulation.getCache(), &simulation.getRam());
            }

            const sim::MemoryTransactionDurations visualDurations = computeVisualLoadDurations(
                previewReadPath, demoConnection->getPath(), demoPrimaryCache->getInstallPath(targetCacheSlotIndex));

            simulation.loadFloat(address, visualDurations);
            activeTransaction = findActiveTransaction();
        }
    }

    if (activeTransaction) {
        syncActiveTransaction(*activeTransaction);
    } else if (registerLoadState.active) {
        advanceRegisterLoad(deltaSeconds);
        syncRegisterLoadState();
    } else {
        syncIdleState();
    }

    camera.update(window);
}

void MachineController::render(sf::RenderWindow& window) const {
    window.clear(kBackgroundColor);
    window.setView(camera.getView());
    window.draw(scene);
    for (const view::BlockView* block : topLevelBlocks) {
        window.draw(*block);
    }
    for (const Connection& connection : connections) {
        if (connection.bus.isVisible()) {
            window.draw(connection.bus);
        }
    }
    window.draw(readAnimation);
    window.draw(registerLoadAnimation);
}

void MachineController::onWindowModeChanged(const sf::RenderWindow& window) {
    sceneView.setCenter(camera.getPosition());
    sceneView.setSize(
        {static_cast<float>(window.getSize().x) / camera.getZoom(),
         static_cast<float>(window.getSize().y) / camera.getZoom()});
}

void MachineController::clearScene() {
    cancelInteraction();
    topLevelBlocks.clear();
    componentViewsById.clear();
    ramViews.clear();
    cpuViews.clear();
    connections.clear();
    readAnimation.clear();
    registerLoadAnimation.clear();
    nextReadLineIndex = 0;
    lastRegisterLoadTransactionId = 0;
    nextRegisterCellIndex = 0;
    zmmLabels.fill("");
    registerLoadState = {};
}

void MachineController::rebuildComponentRegistry() {
    componentViewsById.clear();

    for (auto& [id, ramView] : ramViews) {
        componentViewsById[id] = ramView.get();
    }

    for (auto& [id, cpuView] : cpuViews) {
        componentViewsById[id] = cpuView.get();
        if (view::CacheView* primaryCache = cpuView->getPrimaryCacheView()) {
            componentViewsById[id + ".l1"] = primaryCache;
        }
    }
}

void MachineController::refreshConnections() {
    for (Connection& connection : connections) {
        view::BlockView* fromComponent = findComponent(connection.fromComponentId);
        view::BlockView* toComponent = findComponent(connection.toComponentId);
        if (!fromComponent || !toComponent) {
            connection.bus.clear();
            continue;
        }

        const view::PortView* fromPort = fromComponent->findPort(connection.fromPortId);
        const view::PortView* toPort = toComponent->findPort(connection.toPortId);
        if (!fromPort || !toPort) {
            connection.bus.clear();
            continue;
        }

        connection.bus.connect(*fromPort, *toPort);
        connection.bus.setHighlighted(connection.highlighted);
    }
}

view::RamView* MachineController::getDemoRam() {
    if (auto it = ramViews.find(kDemoRamId); it != ramViews.end()) {
        return it->second.get();
    }

    return nullptr;
}

const view::RamView* MachineController::getDemoRam() const {
    if (auto it = ramViews.find(kDemoRamId); it != ramViews.end()) {
        return it->second.get();
    }

    return nullptr;
}

view::CpuView* MachineController::getDemoCpu() {
    if (auto it = cpuViews.find(kDemoCpuId); it != cpuViews.end()) {
        return it->second.get();
    }

    return nullptr;
}

const view::CpuView* MachineController::getDemoCpu() const {
    if (auto it = cpuViews.find(kDemoCpuId); it != cpuViews.end()) {
        return it->second.get();
    }

    return nullptr;
}

view::CacheView* MachineController::getDemoPrimaryCache() {
    if (view::CpuView* demoCpu = getDemoCpu()) {
        return demoCpu->getPrimaryCacheView();
    }

    return nullptr;
}

const view::CacheView* MachineController::getDemoPrimaryCache() const {
    if (const view::CpuView* demoCpu = getDemoCpu()) {
        return demoCpu->getPrimaryCacheView();
    }

    return nullptr;
}

MachineController::Connection* MachineController::getDemoConnection() {
    for (Connection& connection : connections) {
        if (connection.id == kDemoConnectionId) {
            return &connection;
        }
    }

    return nullptr;
}

const MachineController::Connection* MachineController::getDemoConnection() const {
    for (const Connection& connection : connections) {
        if (connection.id == kDemoConnectionId) {
            return &connection;
        }
    }

    return nullptr;
}

void MachineController::syncIdleState() {
    if (view::RamView* demoRam = getDemoRam()) {
        demoRam->sync(simulation.getRam());
        demoRam->setHighlightedLine(std::nullopt);
        std::array<float, view::CacheLineView::kFloatCount> emptyHighlights{};
        emptyHighlights.fill(0.0f);
        demoRam->setHighlightedCells(std::nullopt, emptyHighlights);
    }

    if (view::CpuView* demoCpu = getDemoCpu()) {
        demoCpu->syncPrimaryCache(simulation.getCache(), &simulation.getRam());
        demoCpu->setRegisterLabels(zmmLabels);
    }

    if (view::CacheView* demoPrimaryCache = getDemoPrimaryCache()) {
        demoPrimaryCache->setHighlightedSlot(std::nullopt);
        demoPrimaryCache->setReadHighlight(std::nullopt, std::nullopt, 0.0f);
    }

    if (Connection* demoConnection = getDemoConnection()) {
        demoConnection->highlighted = false;
        demoConnection->bus.setHighlighted(false);
    }

    readAnimation.clear();
    std::array<float, view::CacheLineView::kFloatCount> emptyReadHighlights{};
    emptyReadHighlights.fill(0.0f);
    readAnimation.setHighlightedCells(emptyReadHighlights);
    registerLoadAnimation.clear();
}

void MachineController::syncActiveTransaction(const sim::MemoryTransaction& activeTransaction) {
    view::RamView* demoRam = getDemoRam();
    view::CpuView* demoCpu = getDemoCpu();
    view::CacheView* demoPrimaryCache = getDemoPrimaryCache();
    Connection* demoConnection = getDemoConnection();
    if (!demoRam || !demoCpu || !demoPrimaryCache || !demoConnection) {
        return;
    }

    const std::size_t lineIndex =
        static_cast<std::size_t>(activeTransaction.getLineBaseAddress() / sim::RAM::kCacheLineSizeInBytes);
    const view::RamView::ReadPath readPath = demoRam->getReadPath(lineIndex);
    demoRam->sync(simulation.getRam());
    demoCpu->syncPrimaryCache(simulation.getCache(), &simulation.getRam(), &activeTransaction);
    demoPrimaryCache = getDemoPrimaryCache();
    if (!demoPrimaryCache) {
        return;
    }

    demoRam->setHighlightedLine(lineIndex);
    std::array<float, view::CacheLineView::kFloatCount> xHighlights{};
    xHighlights.fill(0.0f);
    const sim::RAM::LineCellLabels lineLabels = simulation.getRam().getLineCellLabels(lineIndex);
    for (std::size_t index = 0; index < lineLabels.size(); ++index) {
        if (lineLabels[index].rfind("x[", 0) == 0) {
            xHighlights[index] = 1.0f;
        }
    }
    std::array<float, view::CacheLineView::kFloatCount> emptyRamHighlights{};
    emptyRamHighlights.fill(0.0f);
    demoRam->setHighlightedCells(std::nullopt, emptyRamHighlights);
    demoPrimaryCache->setHighlightedSlot(activeTransaction.getTargetCacheSlotIndex());
    demoPrimaryCache->setReadHighlight(std::nullopt, std::nullopt, 0.0f);
    demoConnection->highlighted = true;
    demoConnection->bus.setHighlighted(true);
    demoCpu->setRegisterLabels(zmmLabels);

    readAnimation.setCellLabels(simulation.getRam().getLineCellLabels(lineIndex));
    readAnimation.setHighlightedCells(xHighlights);
    readAnimation.setRoute(readPath.sourcePosition,
                           readPath.lanePosition,
                           readPath.turnEntryPosition,
                           readPath.turnCenter,
                           readPath.turnRadius,
                           readPath.turnStartAngle,
                           readPath.turnEndAngle,
                           readPath.turnExitPosition,
                           readPath.collectorPosition,
                           readPath.junctionTurnCenter,
                           readPath.junctionTurnRadius,
                           readPath.junctionTurnStartAngle,
                           readPath.junctionTurnEndAngle,
                           readPath.junctionTurnExitPosition,
                           readPath.exitPosition,
                           demoPrimaryCache->getLineHeadCenter());
    readAnimation.sync(activeTransaction,
                       simulation.getCurrentTick(),
                       demoConnection->bus.isVisible() ? &demoConnection->bus.getPath() : nullptr,
                       &demoPrimaryCache->getInstallPath());
    registerLoadAnimation.clear();
}

void MachineController::syncRegisterLoadState() {
    view::RamView* demoRam = getDemoRam();
    view::CpuView* demoCpu = getDemoCpu();
    view::CacheView* demoPrimaryCache = getDemoPrimaryCache();
    Connection* demoConnection = getDemoConnection();
    if (!demoRam || !demoCpu || !demoPrimaryCache || !demoConnection) {
        return;
    }

    demoRam->sync(simulation.getRam());
    demoRam->setHighlightedLine(std::nullopt);
    demoCpu->syncPrimaryCache(simulation.getCache(), &simulation.getRam());
    demoCpu->setRegisterLabels(zmmLabels);
    demoPrimaryCache->setHighlightedSlot(registerLoadState.sourceCacheSlotIndex);
    if (registerLoadState.currentLabelIndex < registerLoadState.sourceCellIndices.size()) {
        demoPrimaryCache->setReadHighlight(registerLoadState.sourceCacheSlotIndex,
                                           registerLoadState.sourceCellIndices[registerLoadState.currentLabelIndex],
                                           std::max(0.25f, registerLoadAnimation.getHighlightProgress()));
    } else {
        demoPrimaryCache->setReadHighlight(std::nullopt, std::nullopt, 0.0f);
    }
    demoConnection->highlighted = false;
    demoConnection->bus.setHighlighted(false);
    readAnimation.clear();
}

void MachineController::startRegisterLoad(const sim::MemoryTransaction& transaction) {
    registerLoadState = {};
    registerLoadState.active = true;
    registerLoadState.sourceTransactionId = transaction.getId();
    registerLoadState.sourceCacheSlotIndex = transaction.getTargetCacheSlotIndex();

    const std::size_t lineIndex =
        static_cast<std::size_t>(transaction.getLineBaseAddress() / sim::RAM::kCacheLineSizeInBytes);
    const sim::RAM::LineCellLabels labels = simulation.getRam().getLineCellLabels(lineIndex);
    for (std::size_t index = 0; index < labels.size(); ++index) {
        const std::string& label = labels[index];
        if (label.rfind("x[", 0) == 0) {
            registerLoadState.labels.push_back(label);
            registerLoadState.sourceCellIndices.push_back(index);
        }
    }

    if (registerLoadState.labels.empty()) {
        registerLoadState.active = false;
        lastRegisterLoadTransactionId = transaction.getId();
        return;
    }

    if (view::CpuView* demoCpu = getDemoCpu()) {
        if (view::CacheView* demoPrimaryCache = getDemoPrimaryCache()) {
            registerLoadAnimation.start(
                buildRegisterLoadPath(*demoPrimaryCache,
                                      registerLoadState.sourceCacheSlotIndex,
                                      registerLoadState.sourceCellIndices.front(),
                                      demoCpu->getCacheToAluPath(),
                                      kRailThickness + 1.0f),
                registerLoadState.labels.front(),
                demoCpu->getRegisterCellCenter(nextRegisterCellIndex));
            if (!registerLoadAnimation.isActive()) {
                lastRegisterLoadTransactionId = transaction.getId();
                registerLoadState = {};
            }
        }
    }
}

void MachineController::advanceRegisterLoad(float deltaSeconds) {
    if (!registerLoadState.active) {
        return;
    }

    const float pixelsPerSecond =
        animationTiming.simulationTicksPerSecond * animationTiming.pixelsPerTick * 0.55f;
    if (!registerLoadAnimation.update(deltaSeconds, pixelsPerSecond)) {
        return;
    }

    if (registerLoadState.currentLabelIndex < registerLoadState.labels.size()) {
        zmmLabels[nextRegisterCellIndex] = registerLoadState.labels[registerLoadState.currentLabelIndex];
        nextRegisterCellIndex = (nextRegisterCellIndex + 1) % view::CacheLineView::kFloatCount;
        registerLoadState.currentLabelIndex += 1;
    }

    if (registerLoadState.currentLabelIndex >= registerLoadState.labels.size()) {
        lastRegisterLoadTransactionId = registerLoadState.sourceTransactionId;
        registerLoadState = {};
        registerLoadAnimation.clear();
        return;
    }

    if (view::CpuView* demoCpu = getDemoCpu()) {
        if (view::CacheView* demoPrimaryCache = getDemoPrimaryCache()) {
            registerLoadAnimation.start(
                buildRegisterLoadPath(*demoPrimaryCache,
                                      registerLoadState.sourceCacheSlotIndex,
                                      registerLoadState.sourceCellIndices[registerLoadState.currentLabelIndex],
                                      demoCpu->getCacheToAluPath(),
                                      kRailThickness + 1.0f),
                registerLoadState.labels[registerLoadState.currentLabelIndex],
                demoCpu->getRegisterCellCenter(nextRegisterCellIndex));
            if (!registerLoadAnimation.isActive()) {
                lastRegisterLoadTransactionId = registerLoadState.sourceTransactionId;
                registerLoadState = {};
            }
        }
    }
}

void MachineController::cancelInteraction() {
    previousLeftMousePressed = false;
    dragTarget = {};
    for (view::BlockView* block : topLevelBlocks) {
        block->setDragState(false, false);
    }
}

const sim::MemoryTransaction* MachineController::findActiveTransaction() const {
    for (auto it = simulation.getTransactions().rbegin(); it != simulation.getTransactions().rend(); ++it) {
        if (!it->isCompleted(simulation.getCurrentTick())) {
            return &(*it);
        }
    }

    return nullptr;
}

sim::Address MachineController::chooseRandomAnimatedAddress() {
    const std::size_t ramLineCount = simulation.getRam().getLineCount();

    if (ramLineCount == 0) {
        return 0;
    }

    const std::size_t lineIndex = nextReadLineIndex % ramLineCount;
    nextReadLineIndex = (lineIndex + 1) % ramLineCount;
    if (lineIndex == 0 && nextReadLineIndex == 1) {
        simulation.getCache().clear();
    }
    return toBlockAddress(lineIndex);
}

sim::Tick MachineController::ticksForDistance(float distance, float pixelsPerTick, sim::Tick minimumTicks) {
    if (distance <= 0.0f) {
        return minimumTicks;
    }

    return std::max(minimumTicks,
                    static_cast<sim::Tick>(std::ceil(distance / std::max(pixelsPerTick, 1.0f))));
}

float MachineController::computeLength(sf::Vector2f vector) {
    return std::sqrt(vector.x * vector.x + vector.y * vector.y);
}

float MachineController::computeRamRouteLength(const view::RamView::ReadPath& readPath) {
    const float laneDistance = computeLength(readPath.lanePosition - readPath.sourcePosition);
    const float toTurnEntryDistance = computeLength(readPath.turnEntryPosition - readPath.lanePosition);
    const float firstTurnDistance =
        std::abs(readPath.turnEndAngle - readPath.turnStartAngle) * readPath.turnRadius;
    const float collectorDistance = computeLength(readPath.collectorPosition - readPath.turnExitPosition);
    const float junctionTurnDistance =
        std::abs(readPath.junctionTurnEndAngle - readPath.junctionTurnStartAngle) * readPath.junctionTurnRadius;
    const float exitDistance = computeLength(readPath.exitPosition - readPath.junctionTurnExitPosition);
    return laneDistance + toTurnEntryDistance + firstTurnDistance + collectorDistance + junctionTurnDistance +
           exitDistance;
}

sim::MemoryTransactionDurations MachineController::computeVisualLoadDurations(
    const view::RamView::ReadPath& readPath,
    const view::rails::RailPath& busPath,
    const view::rails::RailPath& installPath) {
    return {ticksForDistance(computeRamRouteLength(readPath),
                             animationTiming.pixelsPerTick,
                             animationTiming.minToRamPortTicks),
            ticksForDistance(busPath.getLength(),
                             animationTiming.pixelsPerTick,
                             animationTiming.minBusTicks),
            ticksForDistance(installPath.getLength(),
                             animationTiming.pixelsPerTick,
                             animationTiming.minInstallTicks)};
}
