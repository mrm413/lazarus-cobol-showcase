       IDENTIFICATION   DIVISION.
       PROGRAM-ID.      prog.
       DATA             DIVISION.
       WORKING-STORAGE  SECTION.
       01 TEST-VAR PIC 9(2) VALUE 'F1'.
       PROCEDURE        DIVISION.
           DISPLAY TEST-VAR NO ADVANCING
           END-DISPLAY.
           STOP RUN.
