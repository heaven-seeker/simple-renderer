# SimpleRenderer: CPU-based 3D Software Renderer

**SimpleRenderer** — это высокопроизводительный программный рендерер на C++, написанный с нуля. Проект демонстрирует реализацию полного графического конвейера без использования GPU, полагаясь исключительно на вычисления центрального процессора.


## Технические особенности (Key Features)

### Освещение и Шейдеры

* **Tangent Space Normal Mapping:** Реализация попиксельного рельефа с использованием матриц **TBN** (Tangent, Bitangent, Normal) для корректного преобразования векторов в мировое пространство.

<img width="5689" height="5689" alt="diabloSS" src="https://github.com/user-attachments/assets/9b770b1b-3253-44a5-bc6e-f6b2d03345cd" />

* **SSAO (Screen Space Ambient Occlusion):** Динамическое затенение в экранном пространстве. Использует полусферное ядро из 128 выборок и буфер глубины для расчета окклюзии.
*Визуализация SSAO Buffer:*

<img width="5689" height="5689" alt="ssao_raw" src="https://github.com/user-attachments/assets/b014968a-41a0-470b-9569-4a4ad60cbac5" />

* **Toon/Cel Shading:** Стилизованный рендеринг с квантованием освещенности и пост-процессингом на основе фильтра Собеля для детекции границ и создания контуров.

<img width="5689" height="5689" alt="toon" src="https://github.com/user-attachments/assets/5c7b1ab0-4dc5-4046-b029-2e70ad769c06" />

### Ядро системы (Core Engine)

* **Barycentric Rasterizer:** Оптимизированный растеризатор, использующий барицентрические координаты для интерполяции атрибутов вершин. Распараллелен с помощью **OpenMP** для повышения производительности.
*Z-Buffer (Визуализация глубины):*

<img width="5689" height="5689" alt="zbuffer_image" src="https://github.com/user-attachments/assets/92eefd19-e68d-40c8-82ff-9528a01a6015" />

* **AO & Shadow Mapping:** Поддержка **Brute-force AO** (1000 лучей на пиксель) в качестве эталона для сравнения с SSAO.
*AO Brute-force:*

<img width="5689" height="5689" alt="ao_buffer" src="https://github.com/user-attachments/assets/109e1e55-3686-4832-bf39-46239ce4efc4" />

* **CLI Interface:** Гибкая настройка сцены через аргументы командной строки: загрузка нескольких моделей, настройка цветов, освещения и выбор шейдера.

### Описание технических деталей

* **TBN Matrix:** Реализована генерация касательного пространства для каждой вершины, что позволяет использовать карты нормалей в пространстве касательных, обеспечивая корректное освещение при любой ориентации модели в мировых координатах.
* **SSAO Logic:** Используется полусферическое ядро из 128 выборок с весовым затуханием и случайным поворотом вектора шума для устранения эффекта «полос».
* **Post-Processing Pipeline:** После основного прохода рендеринга применяется фильтр Собеля по буферу глубины для создания стилизованной обводки и выполняется размытие для сглаживания теней SSAO.


### Полный список аргументов командной строки

| Флаг | Описание | Значение по умолчанию |
| --- | --- | --- |
| `-m` | Пути к одной или нескольким моделям (.obj) | `obj/diablo3_pose.obj` |
| `-shader` | Тип шейдера: `toon`, `gouraud`, `phong`, `tangentspace`, `wireframe`, `solid`, `random` | `toon` |
| `-ao` | Метод затенения: `ssao`, `bruteforce` или `none` | `ssao` |
| `-c` | Список цветов для моделей (например, `gold`, `red`, `white`) | `orange` |
| `-l` | Вектор направления света `x y z` | `-1.5 0.5 1.5` |
| `-w` / `-s` | Ширина и высота выходного изображения | `2048x2048` |


## Сборка и запуск

### Требования

* Компилятор с поддержкой **C++17** (MSVC, GCC или Clang).
* **CMake** (версия 3.10+).
* **OpenMP** (рекомендуется для распараллеливания расчетов SSAO и растеризации).

### Сборка

```bash
# Генерация файлов сборки
cmake -Bbuild

# Компиляция (на Linux/macOS добавьте -j для ускорения)
cmake --build build --config Release

```

### Примеры запуска

Программа поддерживает гибкую настройку через флаги.

**Windows (PowerShell):**

```powershell
# Запуск с SSAO и Tangent Space Normal Mapping
./build/Release/simplerenderer.exe -m obj/diablo3_pose/diablo3_pose.obj -shader tangentspace -ao ssao -c gold

```

**Linux:**

```bash
./build/simplerenderer -m obj/model.obj -shader toon -ao none -l -1 1 1

```
