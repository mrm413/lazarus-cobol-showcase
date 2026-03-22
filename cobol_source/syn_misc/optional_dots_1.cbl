       IDENTIFICATION  DIVISION
       PROGRAM-ID.     cobol85.
       DATA            DIVISION
       WORKING-STORAGE SECTION
       01 X PIC X *> dot optional here because of cobol85 format
       01 Y PIC X.
       PROCEDURE       DIVISION.
       MAIN SECTION.
           PERFORM P
           STOP RUN *> dot optional here because of cobol85 format
       P.
           GOBACK
