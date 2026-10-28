# ReversiAI by Group 7

This project was created for the course 'Softwarepraktikum: Implementierung heuristischer Algorithmen für Brettspiele' at RWTH Aachen, where we were given the task to implement an AI agent for *Reversi plus X* &mdash; a sophisticated variant of classic Reversi.
*Reversi plus X* introduces variable-sized boards (up to 50x50), support for up to 8 players, special board tiles, multiple game phases as well as transition and override mechanics.
This AI is designed to efficiently evaluate game states by utilizing a bitboard representation for fast move generation and resolution. It uses a modified version of the *Kogge-Stone algorithm* for capture propagation and is highly optimized for performance.

## Testing & Documentation
- Unit testing through **GoogleTest**
- Code documentation through **Doxygen**.
  View the full documentation at https://group7-dc1ab5.pages.rwth-aachen.de
- Code formatting via **clang-format**

## Build and Run
ReversiAI requires the following to build and run:
- CMake 3.16
- Build & Runtime Environment supporting C++17 

To build for release, run the following command from the project's root directory:
```bash
~$ cmake -B build
```
To enable non-static assert statements, build using `-DENABLE_ASSERTIONS=ON`.
To compile and link the optional test library, use `-DLINK_TESTS=ON`.
To link runtime address and undefined behavior sanitizers, use `-DLINK_SANITIZERS=ON`.

A debug build which specifies all flags looks as follows:
```bash
~$ cmake -B build -DENABLE_ASSERTIONS=ON -DLINK_TESTS=ON -DLINK_SANITIZERS=ON
```

To compile the main binary, execute the following command from inside of the generated build folder:
```bash
~/build$ cmake --build .
```

For builds with `-DLINK_TESTS=ON` enabled, all tests can be run via
```bash
~/build$ ctest --output-on-failure
```

The main executable can be found inside the bin folder in the project's root. Lastly, to start the client, run
```bash
~/bin$ ./swp2025_group7 <hostname> <port>
```

## Support
You may contact us at any of these 4 Mails:

david.ichim@rwth-aachen.de

jenrik.meisel@rwth-aachen.de

jesper.nilsson@rwth-aachen.de

fina.schlindwein@rwth-aachen.de 

## Roadmap
We plan on implementing the upcoming 5 assignments successfully. More specifically, we currently intend to further optimize our algorithm for move generation and resolution.

## Contributing
David Ichim 459591 (david.ichim@rwth-aachen.de)

Jenrik Meisel 456942 (jenrik.meisel@rwth-aachen.de)

Jesper Nilsson 457057 (jesper.nilsson@rwth-aachen.de) 

Fina Schlindwein 456038 (fina.schlindwein@rwth-aachen.de)

## Project status
This project is a work in progress

