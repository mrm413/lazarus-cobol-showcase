       IDENTIFICATION   DIVISION.
       PROGRAM-ID.      prog_missing_depending.
       DATA             DIVISION.
       WORKING-STORAGE  SECTION.
       1 F-DAT          PICTURE LX(10).
       PROCEDURE        DIVISION.
           MOVE SPACES TO F-DAT
           STOP RUN.
