# QtPad

QtPad is a lightweight Qt5 desktop app for Linux that creates a virtual Xbox 360-style controller through `uinput`.

It focuses on two things:

- Circular left/right stick simulation with auto-centering
- Two combo actions: `LB + Y` and `LB + RB`

## Screenshot

![QtPad interface preview](IMG/panel.png)

## Features

- Circular drag pads for both analog sticks
- Auto-recenter when the mouse is released
- Minimal interface with no key log panel
- Virtual Xbox 360-style output via Linux `uinput`
- Ready-to-build CMake project layout

## Controls

- Drag inside the left circle to move the left stick
- Drag inside the right circle to move the right stick
- Release the mouse button to return the stick to center
- Click `LB + Y` or `LB + RB` to send the corresponding combo

## Build

### Requirements

- Qt5 Widgets
- CMake 3.16 or newer
- Linux kernel `uinput` module

### Compile

```bash
cmake -S . -B build
cmake --build build
```

### Run

```bash
./build/qtpad
```

## Permissions

QtPad needs access to `/dev/uinput`. If the app fails to start, make sure the module is loaded and that your user has permission to use it.

```bash
sudo modprobe uinput
ls -l /dev/uinput
```

If your distribution does not allow regular users to access `/dev/uinput` by default, you can temporarily run the app with `sudo` or create an appropriate `udev` rule.

## Project Structure

- `src/` - Qt application code and virtual controller backend
- `IMG/` - screenshots and other documentation assets
- `CMakeLists.txt` - build configuration
- `LICENSE` - open source license

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for details.

## Notes

This project is useful for:

- Linux controller input testing
- Virtual gamepad demos
- Automation workflows that expect an Xbox 360-style device
