#!/bin/bash

export MISCR_SRC_DIR=~/arduinosrc
export MISCR_BACKUP_DIR=~/arduinosrc/bkup

touch /tmp/temp.ino
cd /tmp

inotifywait -e close_write,moved_to,create  . |
    while read -r directory events filename; do
        if [ "$filename" = "temp.ino" ]; then
            bkupfilename=$(date +%H%M%S)

            echo "Making backup of current file... "
            mv "$MISCR_SRC_DIR/current.ino" "$MISCR_BACKUP_DIR/$bkupfilename.ino"

            echo "Moving temporary file to arduinosrc directory"
            mv /tmp/temp.ino "$MISCR_SRC_DIR/current.ino"

            if [ -f "$MISCR_SRC_DIR/current.ino" ]; then
                export ARDUINO_DIR=/usr/share/arduino
                export ARDMK_DIR=/usr/share/arduino
                export AVR_TOOLS_DIR=/usr

                cd $MISCR_SCR_DIR

                if [ -f "$MISCR_SRC_DIR/Makefile" ]; then
                    echo "$(tput setaf 5)Commence building and uploading!$(tput sgr0)"
                    make upload
                    cd /tmp
                    /host.sh # Change this to your host.sh pathname
                else
                    echo "$(tput setaf 1)Error$(tput sgr0): Makefile not found."
                    exit 2;
                fi
            else
                exit 1;
            fi
        fi
done
