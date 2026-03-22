       IDENTIFICATION   DIVISION.
       PROGRAM-ID.      prog_errs.
       DATA             DIVISION.
       WORKING-STORAGE  SECTION.
       1 W-LGS.
         2 LGS          PICTURE X(10).
       1 W-DATA.
         2 L-DAT        PICTURE LX(10)    DEPENDING ON LGS.
         2 L-ERR1       PICTURE L9        DEPENDING ON LGS.
         2 L-ERR2       PICTURE XXXXLA(2) DEPENDING ON LGS.
         2 L-ERR3       PICTURE LX(1)     DEPENDING ON LGS JUSTIFIED.
       PROCEDURE        DIVISION.
           STOP RUN.
