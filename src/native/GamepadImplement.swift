import Foundation
import CoreHID

@objc
public class SwiftCode: NSObject {
    private static var gamepads: [Int: VirtualGamepadDevice] = [:]
    private static var nextId: Int = 1
    private static let lock = NSLock()

    @objc public static func createGamepad() -> Int {
        lock.lock()
        defer { lock.unlock() }

        let device = VirtualGamepadDevice()

        // Check if device creation was initiated successfully
        if !device.isInitialized() {
            return -1
        }

        let gamepadId = nextId
        nextId += 1
        gamepads[gamepadId] = device

        return gamepadId
    }

    @objc public static func destroyGamepad(_ gamepadId: Int) -> Bool {
        lock.lock()
        defer { lock.unlock() }

        guard let device = gamepads[gamepadId] else {
            return false
        }

        device.destroy()
        gamepads.removeValue(forKey: gamepadId)
        return true
    }

    @objc public static func buttonDown(_ gamepadId: Int, button buttonId: Int) -> Bool {
        lock.lock()
        defer { lock.unlock() }

        guard let device = gamepads[gamepadId] else {
            return false
        }

        return device.setButton(buttonIndex: buttonId, isDown: true)
    }

    @objc public static func buttonUp(_ gamepadId: Int, button buttonId: Int) -> Bool {
        lock.lock()
        defer { lock.unlock() }

        guard let device = gamepads[gamepadId] else {
            return false
        }

        return device.setButton(buttonIndex: buttonId, isDown: false)
    }

    @objc public static func setAxis(_ gamepadId: Int, axis axisId: Int, value: Int) -> Bool {
        lock.lock()
        defer { lock.unlock() }

        guard let device = gamepads[gamepadId] else {
            return false
        }

        return device.setAxis(axisIndex: axisId, value: Int16(clamping: value))
    }

    // between these, changes are kept and go as one report (setState)
    @objc public static func beginUpdate(_ gamepadId: Int) -> Bool {
        lock.lock()
        defer { lock.unlock() }

        guard let device = gamepads[gamepadId] else {
            return false
        }

        device.beginUpdate()
        return true
    }

    @objc public static func endUpdate(_ gamepadId: Int) -> Bool {
        lock.lock()
        defer { lock.unlock() }

        guard let device = gamepads[gamepadId] else {
            return false
        }

        return device.endUpdate()
    }
}

// The gamepad's state, sent whole in every report
public struct GamepadState {
    var buttons: UInt32        // 17 buttons, W3C Standard Gamepad order
    var thumbLX: Int16
    var thumbLY: Int16
    var thumbRX: Int16
    var thumbRY: Int16
    var leftTrigger: UInt8
    var rightTrigger: UInt8
}

// Delegate to handle HID virtual device callbacks. CoreHID's virtual devices
// are macOS 26 API; the addon itself loads from macOS 10.15 on, where
// create() just reports it cannot make a gamepad.
@available(macOS 26.0, *)
final class GamepadDelegate: HIDVirtualDeviceDelegate {
    func hidVirtualDevice(_ device: HIDVirtualDevice, receivedSetReportRequestOfType type: HIDReportType, id: HIDReportID?, data: Data) throws {
        // Handle set report requests if needed
    }

    func hidVirtualDevice(_ device: HIDVirtualDevice, receivedGetReportRequestOfType type: HIDReportType, id: HIDReportID?, maxSize: size_t) throws -> Data {
        // Return empty data for get report requests
        return Data()
    }
}

// Swift class wrapping HIDVirtualDevice
@objc public class VirtualGamepadDevice: NSObject {
    static let buttonCount = 17

    // a HIDVirtualDevice and its GamepadDelegate, kept untyped as both are
    // macOS 26 types
    private var device: Any?
    private var currentState: GamepadState
    private var delegate: Any?
    // reports go through one stream, to one task, so they reach the system
    // in the order they were made
    private var reports: AsyncStream<Data>.Continuation?
    private var pumpTask: Task<Void, Never>?
    private var initialized: Bool = false
    // while true, changes are kept and not sent (beginUpdate/endUpdate)
    private var isUpdating: Bool = false

    // A generic HID gamepad. The order of the usages is the W3C Standard
    // Gamepad order, which is what browsers and SDL fall back to for a gamepad
    // they have no mapping for: buttons 1-17, then X, Y, Z, Rx for the sticks
    // (left X, left Y, right X, right Y) and Ry, Rz for the triggers.
    private static let reportDescriptor: [UInt8] = [
        0x05, 0x01,        // Usage Page (Generic Desktop)
        0x09, 0x05,        // Usage (Game Pad)
        0xA1, 0x01,        // Collection (Application)
        0x05, 0x09,        //   Usage Page (Button)
        0x19, 0x01,        //   Usage Minimum (Button 1)
        0x29, 0x11,        //   Usage Maximum (Button 17)
        0x15, 0x00,        //   Logical Minimum (0)
        0x25, 0x01,        //   Logical Maximum (1)
        0x75, 0x01,        //   Report Size (1)
        0x95, 0x11,        //   Report Count (17)
        0x81, 0x02,        //   Input (Data, Variable, Absolute)
        0x75, 0x01,        //   Report Size (1)
        0x95, 0x07,        //   Report Count (7)
        0x81, 0x03,        //   Input (Constant) - pads the buttons to 3 bytes
        0x05, 0x01,        //   Usage Page (Generic Desktop)
        0x09, 0x30,        //   Usage (X)
        0x09, 0x31,        //   Usage (Y)
        0x09, 0x32,        //   Usage (Z)
        0x09, 0x33,        //   Usage (Rx)
        0x16, 0x00, 0x80,  //   Logical Minimum (-32768)
        0x26, 0xFF, 0x7F,  //   Logical Maximum (32767)
        0x75, 0x10,        //   Report Size (16)
        0x95, 0x04,        //   Report Count (4)
        0x81, 0x02,        //   Input (Data, Variable, Absolute)
        0x05, 0x01,        //   Usage Page (Generic Desktop)
        0x09, 0x34,        //   Usage (Ry)
        0x09, 0x35,        //   Usage (Rz)
        0x15, 0x00,        //   Logical Minimum (0)
        0x26, 0xFF, 0x00,  //   Logical Maximum (255)
        0x75, 0x08,        //   Report Size (8)
        0x95, 0x02,        //   Report Count (2)
        0x81, 0x02,        //   Input (Data, Variable, Absolute)
        0xC0               // End Collection
    ]

    public override init() {
        self.currentState = GamepadState(
            buttons: 0,
            thumbLX: 0,
            thumbLY: 0,
            thumbRX: 0,
            thumbRY: 0,
            leftTrigger: 0,
            rightTrigger: 0
        )
        super.init()

        // Check if macOS version is supported and create device
        if #available(macOS 26.0, *) {
            // Not an Xbox controller's IDs: the report layout above is not an
            // Xbox controller's, and drivers that know those IDs would read it
            // wrong. 0x1209 is pid.codes, the vendor ID for open projects;
            // 0x0001 is its product ID for testing.
            let properties = HIDVirtualDevice.Properties(
                descriptor: Data(Self.reportDescriptor),
                vendorID: 0x1209,
                productID: 0x0001,
                transport: .usb,
                product: "easy-control Virtual Gamepad",
                manufacturer: "easy-control",
                serialNumber: "EasyControl001"
            )

            guard let hidDevice = HIDVirtualDevice(properties: properties) else {
                self.initialized = false
                return
            }

            let delegate = GamepadDelegate()
            self.device = hidDevice
            self.delegate = delegate
            self.initialized = true

            var continuation: AsyncStream<Data>.Continuation?
            let stream = AsyncStream<Data> { continuation = $0 }
            self.reports = continuation

            // activate, then send the reports one after another; reports made
            // before the activation finishes wait in the stream
            pumpTask = Task { [weak self] in
                do {
                    try await hidDevice.activate(delegate: delegate)
                } catch {
                    self?.initialized = false
                    return
                }
                for await report in stream {
                    try? await hidDevice.dispatchInputReport(data: report, timestamp: SuspendingClock.now)
                }
            }
        } else {
            self.initialized = false
        }
    }

    @objc public func isInitialized() -> Bool {
        return initialized
    }

    @objc public func destroy() {
        reports?.finish()
        reports = nil
        pumpTask?.cancel()
        pumpTask = nil
        device = nil
        delegate = nil
        initialized = false
    }

    // buttons 6 and 7 are the triggers: they also pull the analog trigger
    @objc public func setButton(buttonIndex: Int, isDown: Bool) -> Bool {
        guard buttonIndex >= 0 && buttonIndex < Self.buttonCount else {
            return false
        }
        let mask: UInt32 = 1 << UInt32(buttonIndex)
        if isDown {
            currentState.buttons |= mask
        } else {
            currentState.buttons &= ~mask
        }
        if buttonIndex == 6 {
            currentState.leftTrigger = isDown ? 255 : 0
        } else if buttonIndex == 7 {
            currentState.rightTrigger = isDown ? 255 : 0
        }
        return sendReport()
    }

    @objc public func setAxis(axisIndex: Int, value: Int16) -> Bool {
        switch axisIndex {
        case 0:
            currentState.thumbLX = value
        case 1:
            currentState.thumbLY = value
        case 2:
            currentState.thumbRX = value
        case 3:
            currentState.thumbRY = value
        case 4:
            currentState.leftTrigger = UInt8(max(0, min(255, (Int(value) + 32768) / 256)))
        case 5:
            currentState.rightTrigger = UInt8(max(0, min(255, (Int(value) + 32768) / 256)))
        default:
            return false
        }
        return sendReport()
    }

    @objc public func beginUpdate() {
        isUpdating = true
    }

    @objc public func endUpdate() -> Bool {
        isUpdating = false
        return sendReport()
    }

    private func sendReport() -> Bool {
        guard initialized, let reports = reports else {
            return false
        }
        if isUpdating {
            return true
        }

        // 3 bytes of buttons + 4 x 16-bit sticks + 2 x 8-bit triggers
        var report = Data(capacity: 13)

        let buttons = currentState.buttons
        report.append(UInt8(buttons & 0xFF))
        report.append(UInt8((buttons >> 8) & 0xFF))
        report.append(UInt8((buttons >> 16) & 0xFF))

        withUnsafeBytes(of: currentState.thumbLX.littleEndian) { report.append(contentsOf: $0) }
        withUnsafeBytes(of: currentState.thumbLY.littleEndian) { report.append(contentsOf: $0) }
        withUnsafeBytes(of: currentState.thumbRX.littleEndian) { report.append(contentsOf: $0) }
        withUnsafeBytes(of: currentState.thumbRY.littleEndian) { report.append(contentsOf: $0) }

        report.append(currentState.leftTrigger)
        report.append(currentState.rightTrigger)

        reports.yield(report)
        return true
    }
}
