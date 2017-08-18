FAQ
===

#### What are the username and password for the Debian testing image?

icfp/icfp

#### When the robot moves through earth (“.”), does he replace it with empty (” “)?

Yes – as explained at the end of section 2.2 in the spec, after a successful move, the grid location (x, y) becomes Empty.

#### Can you give some examples of rocks falling?

In the following situation, if the robot moves left, it will be squashed by the falling rock, because it has moved into a location below a rock which has just fallen:

```
#*. #
# ..#
# R #
#####
```

In the following situation, if the robot moves left, it is safe, because robot movement happens before map update, and there is no empty space for the rock to fall into.

```
#*. #
# R #
#####
```

In the following sitation, if the robot waits, it is safe, because even though there is a rock on top of a rock which would otherwise fall into an empty square, the square it would fall into is occupied by the robot.

```
#   #
# * #
#.*R#
#####
```

If the Robot is underneath a Rock, and moves down, it will be squashed. There is no way to outrun a Rock. For example, if the Robot moves down here, it will be squashed by a falling rock:

```
#*  #
#R  #
#...#
#####
```

#### What happens if a beard grows and a rock falls into the same cell during an update?

Update works across the cells from left to right, bottom to top, on the old state, so the effect caused by the higher, rightmost update will take precedence.

#### What happens if the Robot walks off the edge of the map?

All of our test maps are surrounded by Walls or Lambda Lifts, so this should be impossible. However, there is an implicit wall around the edge so if a Robot attempts to go off the edge of a map, it will execute a Wait instead.

#### Is it possible to have a negative score?

Yes. The score starts at zero, so after the Robot’s first move, unless it collects a Lambda, its score will move to -1.

#### How big can the maps get?

We’ve left this unspecified – try to cope with as large a map as you can within the constraints of 1Gb. We have some test maps of 1000×1000, but we may make bigger ones…

#### How accurate is the web validator?

The web validator uses the same code to validate routes as the scoring system which will be used to judge the contest.

#### Does waiting (W) cost you a point? What about aborting (A)?

Waiting does cost a point. Trying to move into an immovable object (e.g. moving left when a wall is on your immediate left) will cost you 1 point as well (the same as waiting or moving, but no extra for having a failed move).

Aborting does not cost anything at all, regardless of whether you explicitly abort, or your commands finish without the robot getting to the open lift.

#### So abort (A) isn’t really needed?

Indeed not – Instead you could just stop giving out commands after when you would have sent an Abort.

#### What happens if a Robot goes up just as the water level rises?

If this happens, it is as if the robot left the water, then re-entered the water.

#### Can a robot step on a target?

Targets and trampolines act exactly like walls, except that a robot can step on a trampoline and be transported to a target. So, no, a robot can not step on a target.

#### How do I use sudo in my install script without a password?

We will turn off the password for sudo before processing submissions.  You can do this in your own VM by running the ‘sudo visudo’ command and changing the line for the sudo group to

```
%sudo  ALL=(ALL:ALL) NOPASSWD: ALL
```

Then Ctrl+O and Return to save it, and Ctrl+X to exit.

#### What’s the easiest way to copy files to a VirtualBox VM?

Use the ‘shared folders’ feature of VirtualBox.  You can add a shared folder from the Shared Folders option in the Devices menu, and mount it on the virtual machine using

```
sudo mount -t vboxsf <shared folder name> -o uid=1000 /mnt
```

which will make the shared folder appear in the /mnt directory.

#### How can I use F# in the VM?

Now that the contest is well underway we’ve had a chance to play with F# on the judging VM. Having passed the rigorous test of printing “Hello, world!” we’ve decided to share [a version of the VM with F# preinstalled in it](http://www-fp.cs.st-andrews.ac.uk/~icfppc/ICFPcontest-FSharp.ova) (1.3GB).

#### Can I use a package from Debian experimental (e.g. pypy)?

As packages in experimental might change quite quickly, we think it would be best if you included a copy of the packages in your submission (i.e., the .deb files for pypy, pypy-lib, etc) and add an “install” script (something like “sudo dpkg -i pypy*.deb” should work).
