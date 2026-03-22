       IDENTIFICATION   DIVISION.
       PROGRAM-ID.      prog.
       DATA             DIVISION.
       WORKING-STORAGE  SECTION.
       01 X             PIC X(05).
       PROCEDURE        DIVISION.
           ACCEPT X WITH LOWER     END-ACCEPT.
           ACCEPT X WITH UPPER     END-ACCEPT.
           ACCEPT X      LOWER
           ACCEPT X      UPPER
           STOP RUN.
