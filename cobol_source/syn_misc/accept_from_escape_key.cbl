       IDENTIFICATION   DIVISION.
       PROGRAM-ID.      prog.
       DATA             DIVISION.
       WORKING-STORAGE  SECTION.
       77 KEYNUM        PIC 9(04).
       PROCEDURE        DIVISION.

           ACCEPT KEYNUM FROM ESCAPE KEY
           DISPLAY "Key pressed: " KEYNUM
           ACCEPT KEYNUM FROM ESCAPE
           DISPLAY "Key pressed: " KEYNUM
           ACCEPT OMITTED

           STOP RUN.
