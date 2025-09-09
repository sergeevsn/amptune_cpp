# Seismic Amplification Tuning Tool

A tool for tuning seismic data amplitudes with interactive area selection.

## Features

- Load and save SEG-Y files
- Interactive area selection (polygon or rectangle)
- Amplitude scaling with adjustable coefficient
- Smooth transitions at selected area boundaries
- Operation history with undo/redo capability
- Seismic data visualization as an image

## Building

```bash
mkdir build
cd build
cmake ..
make
```

## Usage

1. **Load Data**: Click "Load SEG-Y File" to load seismic data
2. **Select Area**: 
   - Choose selection mode (Point by Point or Rectangle)
   - Select area on the image
   - Right-click to apply processing
3. **Configure Parameters**:
   - Scale Factor: scaling coefficient (0.1 - 20.0)
   - Transition Traces: transition zone width in traces
   - Transition Time: transition zone width in milliseconds
   - Transition Mode: transition mode (inside/outside)
4. **Save**: Click "Save SEG-Y File" to save the result

## Controls

- **Left Mouse Button**: add selection points / start rectangle selection
- **Right Mouse Button**: finish selection and apply processing
- **Escape**: clear current selection
- **Enter**: finish polygon selection

## Operation History

- **Undo**: undo last operation
- **Redo**: redo undone operation
- **Reset**: return to original data

## Technical Details

- **Language**: C++11
- **GUI**: Qt5
- **Data Format**: SEG-Y
- **Algorithm**: Scaling with smooth transitions based on distance transform

## Project Structure

```
src/
├── gui/           # User interface
├── amplify/       # Processing algorithms
└── ioutils/       # SEG-Y file I/O
```
