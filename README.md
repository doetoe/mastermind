# Mastermind #

This program is a Mastermind playing assistant. The objective is to play an optimal strategy, which may not be the case yet. At present, in every situation it computes the intent of higest entropy, i.e. the highest expected information gain. For large numbers of colors and positions, it still is too slow, though several relatively easy improvements could be implemented. Note that the first step is the slowest, each subsequent guess is much faster as the number of possibilities gets reduced.

It is designed so that it could be used as a library, although at present it is compiled into an interactive executable.

### Usage ###

Compile the executable by invoking `make`. This will generate the program `mastermind`. When run without, usage information is displayed, namely 

    Usage: mastermind colors positions

* `colors` is a string containing the colors you're playing with, like `rgbcmy`, `12345678` or `rygbovBc`.
* `positions` is the number of positions.

When executing, it will enter into a dialog mode. Before each of your intents, it will suggest you an optimal move. You enter your actual intent (either the suggested one or not), along with the evaluation (how many black/white) your opponent gave:

    intent black white> rygb 1 2

This means you tried the combination `rygb` and this resulted in 1 black, 2 white. A suggestion will be given in the same format, e.g.

    You could try rgog

except for the first one, which will be something like `2,1,1`, meaning that the optimal move is to try two equal colors, and two other ones.

### Contact ###

doetoe@protonmail.com
