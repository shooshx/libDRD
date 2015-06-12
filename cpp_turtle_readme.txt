This example is a simple "LOGO" like drawing program
- See code for the list of interpreted commands
- lines are drawn pixel-by-pixel




##### simple circle
do 360
  F 2
  T 1
next

##### print to the small output window
= a 0
do 10
  print a
  + a 1
next

##### color circle of squares
= r 100
= g 160
do 90
  do 4
    F 160
    T 90
    color r g 255
  next
  + r 10
  + g 20
  T 4
next

##### circle of pentagons
T 90
Fs 50 # center shape
do 22
  do 5
    F 100
    T 72
  next
  B 100
  T 50
next


##### recursive binary tree
# play with the scrollbars to see the effect
# works better if compiled in "Release"

= ang scroll0
neg ang
= anf scroll1
neg anf
= dang ang
+ dang anf 
neg dang

func tr lvl startf
  if lvl < 1
    return
  endif
  = nlvl lvl
  * nlvl 0.65  # Change this to 0.5 to make it run faster
  T ang
  F lvl
  tr nlvl
  B lvl
  T dang
  F lvl
  tr nlvl
  B lvl
  T anf
endf

tr 100