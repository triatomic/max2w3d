# max2w3d / W3D Tools Source Build Instructions

The source code to `max2w3x.dle`, `max2w3d.dle`, `wdump.exe` and `memorymanager.dll` is included with this package.

## Requirements

To compile this project out-of-the-box, you will need:

- Microsoft Visual Studio 2022  
  - Community Edition works fine  
  - Latest patch is recommended  

- 3D Studio Max 2023 SDK  

- Microsoft DirectX SDK  

## DirectX SDK Setup

Download and install the Microsoft DirectX SDK:

https://www.microsoft.com/en-au/download/details.aspx?id=6812

After installation:

### Copy Header Files

Go to the DirectX SDK install folder:

```text
<DirectX SDK>\Include
```

Copy the following files into:

```text
dep\dxsdk_june10\include
```

Files to copy:

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

> Do **not** copy any other files.

### Copy Library File

Go to:

```text
<DirectX SDK>\Lib\x64
```

Copy:

- `d3dx9.lib`

into:

```text
dep\dxsdk_june10\lib\x64
```

> Copy **only** `d3dx9.lib`.

---

## 3D Studio Max SDK Setup

Copy the following folders from the 3D Studio Max 2023 SDK:

- `include`
- `lib`

into:

```text
dep\maxsdk
```

in the source tree.

---

## Build Instructions

Open the solution:

```text
tt.sln
```

and compile using **Visual Studio 2022**.

If you are unable to get it to compile, contact:

- **jonwil** on the W3D Hub forums  
- **Jonathan Wilson** on the W3D Hub Discord  

---

## License

This code is licensed under the **GNU GPL v3.0** as described in:

```text
gpl-3.0.txt
```

### Additional Linking Exemption

In addition to GNU GPL v3.0, you are granted a specific exemption allowing linking of the source code for `max2w3d.dle` with binaries from any 3D program (including 3rd-party plugins), provided that:

- the source code to your modified version of `max2w3d.dle`
- or any plugin including code from `max2w3d.dle`

is released in accordance with **GNU GPL v3.0**.

---

## Building Latest Source

Clone the repository:

```bash
git clone -b wwskin-bundle https://github.com/triatomic/max2w3d
```

Then compile using **Visual Studio 2022** and follow the setup steps above.
