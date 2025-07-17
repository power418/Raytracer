# Belajar-D3D12Engine-Cpp
## Bagaimana untuk menjalankan program sederhana D3D12



1. Pastikan mempunyai text editor berbasis IDE Visual Studio yang telah terinstall di Windows dan sudah menginstallkan terdapat Windows SDK berada Visual Studio Installer (WAJIB!!!).
2. Salin repo project Belajar-D312Engine-Cpp di direktori penyimpanan project anda.
    ```pwsh
    git clone https://github.com/Programmer-nosleep/Belajar-D312Engine-Cpp.git
    ```
3. Bangun project dengan command pada CMake:
    compiler clang:
    ```cmake
    cmake -S src -B build .
    ``` 
    compiler MSVC:
    ```cmake
    cmake -S src -B build .
    ```

4. Masuk ke subdirektori build:
    compiler clang:
    ```pwsh
    cd build; make
    ```

5. Jalankan program yang telah di bangun dan di kompilasikan berada di path ..\build -> .\Debug:
   ```pwsh
   .\Debug\Belajar-D312Engine-Cpp.exe
   ```

Program ini akan menampilkan sebuah program hasil dari grafik render dari D3D12.

## Project Directory Structure

```
Belajar-D312Engine-Cpp/
│
├── build/
├── Debug/
│   └── Belajar-D312Engine-Cpp.exe
├── include/
├── src/
│   ├── CMakeLists.txt
│   └── main.cpp
├── utils/
├── CMakeLists.txt
└── README.md
```