# Rondo

**Game Boy emulator written in C** - a personal, in-progress project striving for clarity, accuracy, and fun.

<br>

##  About Rondo 

- A **work-in-progress**, personal project aiming to recreate the Game Boy experience through C and Libretro support.
- Built from the ground up, featuring core components like the CPU, APU, GPU, and more.
- Supports integration as a [Libretro](https://www.libretro.com/) core for flexible frontend compatibility.

<br>

##  Features

- **Complete emulation stack**: CPU, APU (sound), LCD rendering, and memory mapping.
- **Cross-platform support** via the Libretro interface (e.g., Emulators like RetroArch).
- Built with performance and modularity in mind.
- **Actively under development** - expect new features, improvements, and bug fixes as the project evolves.

<br>

##  Getting Started

### Prerequisites

- A C compiler (e.g., `gcc` or `clang`)
- [Libretro development files](https://github.com/libretro) (if compiling as a Libretro core)
- Make or similar build tools

### Build Instructions

```bash
# Clone the repository
git clone https://github.com/rjgabel/Rondo.git
cd Rondo

# Compile the core (adjust flags as needed)
make

# Or open the Visual Studio solution
# Launch Rondo.sln in Visual Studio and build from the IDE
