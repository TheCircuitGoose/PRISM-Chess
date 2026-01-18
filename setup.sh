#!/bin/bash

# PRISM Engine V0.7
# Tournament Configurator
# (C) 2025 Tommy Ciccone All Rights Reserved.

echo "Prism Engine V0.7"
echo "Tournament Configurator"
echo "(C) 2025 Tommy Ciccone All Rights Reserved."

mkdir -p bots

while true; do
    echo "Select operation:"
    echo "1. Generate new bots"
    echo "2. Mutate existing bot"
    echo "3. Run tournament"
    echo "4. Backup bots"
    echo "5. Exit"
    echo ""
    read -p "Choice: " choice
    echo ""

    case $choice in
        1)
            read -p "Quantity: " count
            echo "Generating $count bots..."
            ./generate "$count" "./bots/"
            echo "Generated $count bots in ./bots/"
            echo ""
            ;;
        2)
            echo ""
            read -p "Input bot path " input_bot
            read -p "Quantity: " count
            read -p "Mutation factor: " mutate_factor
            
            if [ -f "$input_bot" ]; then
                echo "Mutating $input_bot into $count variants..."
                cp "$input_bot" "bots/bot_0"
                echo "Renamed input bot to bot_0"
                
                ./mutate "$input_bot" "$count" "$mutate_factor" "./bots/"
                
                echo "Created $count mutated bots"
                echo ""
            else
                echo "Bot not found."
                echo ""
            fi
            ;;
        3)
            echo "Running tournament with bots in ./bots/"
            ./tournament "./bots/"
            echo ""
            ;;
        4)
            timestamp=$(date +%Y%m%d%H%M%S)
            backup_name="bots_${timestamp}"
            if [ -d "bots" ]; then
                mv bots "$backup_name"
                echo "Bots backed up to $backup_name"
                mkdir -p bots
            else
                echo "No bots to back up"
            fi
            echo ""
            ;;
        5)
            exit 0
            ;;
        *)
            echo "Invalid choice."
            echo ""
            ;;
    esac
done
