# -*- makefile -*-

dominosa : [X] GTK COMMON dominosa dominosa-icon|no-icon

dominosa : [G] WINDOWS COMMON dominosa dominosa.res|noicon.res

ALL += dominosa

!begin gtk
GAMES += dominosa
!end

!begin >list.c
    A(dominosa) \
!end

!begin >wingames.lst
dominosa.exe:Dominosa
!end
