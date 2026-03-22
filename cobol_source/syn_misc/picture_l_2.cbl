       IDENTIFICATION   DIVISION.
       PROGRAM-ID.      prog_value.
       DATA             DIVISION.
       WORKING-STORAGE  SECTION.
       1 L              COMP-1 VALUE 10.
       1 F-DAT          PICTURE LX(10) DEPENDING ON L
                        VALUE "0123456789".
       1 F-DAT2         PICTURE LX(10) DEPENDING ON L
                        BLANK WHEN ZERO.
       PROCEDURE        DIVISION.
           STOP RUN.
