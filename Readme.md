# max2w3d / W3D Tools

Source code for:

- `max2w3x.dle`
- `max2w3d.dle`
- `wdump.exe`
- `memorymanager.dll`

These tools provide W3D export and utility functionality for 3ds Max 2023.

---

## Installation

1. Download both files from the latest release.

2. Place them in your 3ds Max 2023 plugins folder  
   (usually located at):

```text
C:\Program Files\Autodesk\3ds Max 2023\Plugins\
```

---

## Getting the Source

Clone the latest branch:

```bash
git clone --branch wwskin-bundle-WWSkinExporterFixW3DSettUIfix https://github.com/triatomic/max2w3d.git
```

Enter the project folder:

```bash
cd max2w3d
```

---

## Building from Source

### Requirements

- Visual Studio 2022  
  - Community Edition supported
  - Latest updates recommended

- 3ds Max 2023 SDK
- Microsoft DirectX SDK (June 2010)

---

## DirectX SDK Setup

Download and install the DirectX SDK:  
https://www.microsoft.com/en-au/download/details.aspx?id=6812

Copy these files from `<DirectX SDK>\Include` to `dep\dxsdk_june10\include`:

- `d3dx9.h`
- `d3dx9anim.h`
- `d3dx9core.h`
- `d3dx9effect.h`
- `d3dx9math.h`
- `d3dx9math.inl`
- `d3dx9mesh.h`
- `d3dx9shader.h`
- `d3dx9shape.h`
- `d3dx9tex.h`
- `d3dx9xof.h`

Copy `<DirectX SDK>\Lib\x64\d3dx9.lib` to `dep\dxsdk_june10\lib\x64`.

## 3ds Max SDK Setup

Copy `include` and `lib` from the 3ds Max 2023 SDK to `dep\maxsdk`.
inside the source tree.

---

## Compiling

Open:

```text
tt.sln
```

Build using **Visual Studio 2022**.

---

## Support

If you encounter build issues:

- **jonwil** on W3D Hub forums
- **Jonathan Wilson** on W3D Hub Discord

---

## License

Licensed under **GNU GPL v3.0**.

See:

```text
gpl-3.0.txt
```

### Linking Exemption

A specific exemption is granted allowing linking of `max2w3d.dle`
with binaries from any 3D application or third-party plugins, provided
that any modified version of `max2w3d.dle` or derivative plugin
containing code from this project is distributed under GPL v3.0.

---

## Disclaimer

This is an unofficial community project based on reverse-engineered W3D tools.

Portions of this project include code released by Electronic Arts Inc.
under GNU GPL v3.0 relating to WWSkin bone mesh functionality.

This project is not affiliated with, endorsed by, or sponsored by
Electronic Arts Inc.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND.
