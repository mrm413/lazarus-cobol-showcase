       CONTROL          DIVISION.
       REPLACE ==TEST-VAR== BY ==VAR==.
       IDENTIFICATION   DIVISION.
       PROGRAM-ID.      replace.
       DATA             DIVISION.
       WORKING-STORAGE  SECTION.
       01 TEST-VAR PIC X(2) VALUE "OK".
       PROCEDURE        DIVISION.
           DISPLAY VAR NO ADVANCING
           END-DISPLAY.
           STOP RUN.
