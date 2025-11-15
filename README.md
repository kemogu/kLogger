# kLogger (kemogu's Logger)

![Licence](https://img.shields.io/badge/license-MIT-blue.svg)
![C++ Version](https://img.shields.io/badge/C%2B%2B-17-green.svg)
![Structure](https://img.shields.io/badge/structure-header--only-brightgreen.svg)

**[English](#-klogger-english) | [Türkçe](#-klogger-türkçe)**

---

## kLogger (English)

**kLogger (kemogu's Logger)** is a fast, modern, and flexible **header-only** logging library for C++. This project aims to provide developers with a high-performance, easy-to-read logging solution that can be integrated into any C++ application with minimal effort.

### ✨ Features

* **Header-Only:** No compilation needed. Just include the headers and you're ready to go.
* **Lightweight & Fast:** Designed with performance-critical applications in mind.
* **Multi-level:** Standard logging levels (`DEBUG`, `INFO`, `WARN`, `ERROR`, `CRITICAL`).
* **Flexible Formatting:** Easily customize the format of your log messages.
* **Thread-Safe:** Safe to use in multi-threaded applications.
* **Multiple Sinks:** Direct logs to the console, files, or [other targets].

---

### 🚀 Getting Started

#### Requirements

* A **C++17** (or newer) compatible compiler (GCC, Clang, MSVC).

#### Installation

`kLogger` is a **header-only** library. No build process is required.

1.  **Download:** Clone the repository or download the latest release.
    ```bash
    git clone [https://github.com/kemogu/kLogger.git](https://github.com/kemogu/kLogger.git)
    ```
2.  **Include:** Add the `kLogger/include` directory to your project's include path.

**CMake Example (in your `CMakeLists.txt`):**

Simply tell CMake where to find the headers.

```cmake
# Add this to your CMakeLists.txt
# (Assuming you've placed the kLogger repo in your project's 'lib' folder)
target_include_directories(YOUR_PROJECT_NAME PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/kLogger/include
)

# That's it! Now you can #include "kLogger/Logger.h" in your source files.
```
---

## kLogger (Türkçe)

**kLogger (kemogu's Logger)**, C++ için yazılmış hızlı, modern ve esnek bir **header-only** (sadece başlık dosyası) günlükleme (logging) kütüphanesidir. Bu proje, geliştiricilere yüksek performanslı, okunması kolay ve herhangi bir C++ uygulamasına minimum çabayla entegre edilebilen bir günlükleme çözümü sunmayı amaçlamaktadır.

### ✨ Özellikler

*   **Header-Only:** Derleme gerektirmez. Sadece başlık dosyalarını dahil edin ve kullanmaya başlayın.
*   **Hafif ve Hızlı:** Performansın kritik olduğu uygulamalar düşünülerek tasarlanmıştır.
*   **Çok Seviyeli:** Standart günlükleme seviyeleri (`DEBUG`, `INFO`, `WARN`, `ERROR`, `CRITICAL`).
*   **Esnek Formatlama:** Günlük mesajlarınızın formatını kolayca özelleştirin.
*   **Thread-Safe (İş Parçacığı Güvenli):** Çok iş parçacıklı (multi-threaded) uygulamalarda güvenle kullanılabilir.
*   **Çoklu Hedef (Sink):** Günlükleri konsola, dosyalara veya [diğer hedeflere] yönlendirin.

---

### 🚀 Başlarken

#### Gereksinimler

*   **C++17** (veya daha yeni) uyumlu bir derleyici (GCC, Clang, MSVC).

#### Kurulum

`kLogger`, bir **header-only** kütüphanedir. Herhangi bir derleme işlemi gerektirmez.

1.  **İndirme:** Depoyu klonlayın veya en son sürümü indirin.
    ```bash
    git clone https://github.com/KULLANICI_ADINIZ/kLogger.git
    ```
2.  **Dahil Etme:** `kLogger/include` dizinini projenizin dahil etme yoluna (include path) ekleyin.

**CMake Örneği (`CMakeLists.txt` dosyanız için):**

CMake'e başlık dosyalarını nerede bulacağını söylemeniz yeterlidir.

```cmake
# Bunu CMakeLists.txt dosyanıza ekleyin
# (kLogger deposunu projenizin 'lib' klasörüne yerleştirdiğinizi varsayarsak)
target_include_directories(PROJE_ADINIZ PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/kLogger/include
)

# Hepsi bu kadar! Artık kaynak dosyalarınızda #include "kLogger/Logger.h" kullanabilirsiniz.
```
