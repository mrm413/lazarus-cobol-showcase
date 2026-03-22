       IDENTIFICATION   DIVISION.
       PROGRAM-ID.      prog.
       DATA             DIVISION.
       WORKING-STORAGE  SECTION.
       01 X             PIC X(05).
       01 Y             PIC 9(04) BINARY VALUE 4.
       PROCEDURE        DIVISION.
           ACCEPT X WITH           SIZE    0 END-ACCEPT.
           ACCEPT X WITH           SIZE IS 1 END-ACCEPT.
           ACCEPT X WITH PROTECTED SIZE    2 END-ACCEPT.
           ACCEPT X WITH PROTECTED SIZE IS 3 END-ACCEPT.
           ACCEPT X                SIZE    Y END-ACCEPT.
           ACCEPT X                SIZE    0
           ACCEPT X                SIZE IS 1
           ACCEPT X      PROTECTED SIZE    2
           ACCEPT X      PROTECTED SIZE IS 3
           ACCEPT X                SIZE    Y
           STOP RUN.
