Peltar: 2D gravity-based space combat game
==========================================

> **Note**: This game is unfinished and not actively developed.

Peltar is a game for two players who must each try to shoot the other player's
ship.  Missiles are affected by the gravity of the intervening planets.  It
is based on an
[Acorn Archimedes](https://en.wikipedia.org/wiki/Acorn_Archimedes) game called
[Planet Warfare](https://github.com/tlsa/peltar/issues/5#issuecomment-955704718)
that I played when I was much younger.

![Peltar screenshot](https://repository-images.githubusercontent.com/415364809/75c1dfef-ab2a-42e3-9ef4-18559ae0aa8a)

It was developed on Linux in C using SDL.

Why?
----

Before I got a job, I bought "Texturing and Modeling: A Procedural Approach",
and I wanted to experiment with making a fixed-point
[Perlin noise](https://en.wikipedia.org/wiki/Perlin_noise) generator.

I also had an idea for rendering 3D spheres using 2D techniques.  (Basically
drawing a circle and indexing into a texture with a bit of lookup-tabled maths
to select a pixel colour that would give the illusion of 3D.)

It is unfinished, although it is playable.  Most of what remains to do is
main menu, setting up a game, score keeping, etc.

Compile and run
---------------

To build, type:

```
make
```

To build the test programs I used to develop individual features, run:

```
make test
```

To play the game, run:

```
./peltar
```

You can run in fullscreen mode with the `-f` command line flag.
The window width/height can also be set via the command line:

```
./peltar -w800 -h600
```

Playing
-------

It's a two player turn-based game on a single computer.

* The mouse is used to set a firing direction.
  There are little crosshairs next to the player ship
  (a sphere, because I had already written a sphere renderer).
* The strength of the shot is controlled using the mouse scroll
  wheel, and is indicated by the bar at the top of the screen.
* Fire by clicking the mouse button.

The screen border colour indicates which player's turn it is.

The game ends when either player ship is hit.

### Fooling around

Since it was primarily made for playing around with graphics and procedural
generation, there are a few keys that can be used to change things.

| Key | Action                                   |
| --- | ---------------------------------------- |
| `a` | Toggle planet rotation animation on/off. |
| `b` | Generate a new background star-scape.    |
| `l` | Toggle the 3D lighting effect on/off.    |
| `t` | Generate new planet textures.            |
| `z` | Toggle the zoomed-out view.              |

There are also several test programs.  My favourites are:

* `./test-texture` shows an Earth texture I found on the internet and my
  attempt to generate something close to an Earth-like planet.  It was
  helpful while tweaking the texturing code.  The `t` key generates a new
  random planet texture.  I never got around to adding ice caps at the poles.

* `./test-planet` was the first thing written to test the sphere rendering
  algorithm.  If run with any command line argument, it will render a fixed
  number of frames of planet rotation and then exit.  I used it to benchmark
  different optimisations for performance.
