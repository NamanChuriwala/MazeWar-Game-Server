# MazeWar-Game-Server
"MazeWar" is a real-time network game in which the players control avatars
that move around a maze and shoot lasers at each other.
A maze is a two-dimensional array, each cell of which can be occupied by
an avatar, a solid object (i.e. a "wall"), or empty space.
At any given time, each avatar has a particular location in the maze and a
particular direction of gaze.  There are four such directions: NORTH,
SOUTH, EAST, and WEST.  An avatar can be moved forward and backward
in the direction of gaze, and the direction of gaze can be rotated "left"
(counterclockwise) and "right" (clockwise).  An avatar can only be moved
into a cell that was previously empty; if an attempt is made to move into
a cell that is occupied, either by another avatar or a wall, then the
avatar is blocked and the motion fails.
Each avatar is equipped with a laser that it can be commanded to fire.
The laser beam shoots in the direction of gaze and it will destroy the
first other avatar (if any) that it encounters.
The object of the game is to command your avatar to move around the maze,
shoot other avatars, and accumulate points.

The MazeWar server architecture is that of a multi-threaded network server.
When the server is started, a master thread sets up a socket on which to
listen for connections from clients.  When a connection is accepted,
a client service thread is started to handle requests sent by the client
over that connection.  The client service thread executes a service loop in which
it repeatedly receives a request packet sent by the client, performs the request,
and possibly sends one or more packets in response.  The server will also
send packets to the client as the result of actions performed by other users.
For example, if one user moves their avatar into "view" of another user's
avatar, then packets will be sent by the server to the second user's client to
cause the display to update to show the change.
