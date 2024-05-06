#!/usr/bin/python3

import os
import subprocess
import sys

def generate_histogram(directory):
    file_types = {
        'regular': 0,
        'directory': 0,
        'fifo': 0,
        'socket': 0,
        'symbolic-link': 0,
        'block': 0,
        'character': 0
    }

    for root, dirs, files in os.walk(directory):
        for name in files:
            file_path = os.path.join(root, name)
            print(file_path)
            if os.path.islink(file_path):
                file_types['symbolic-link'] += 1
            elif os.path.isfile(file_path):
                file_types['regular'] += 1
            elif os.path.isfifo(file_path):
                file_types['fifo'] += 1
            elif os.path.issocket(file_path):
                file_types['socket'] += 1
            elif os.path.isblock(file_path):
                file_types['block'] += 1
            elif os.path.ischar(file_path):
                file_types['character'] += 1

        for dir in dirs:
            print(dir)
            file_types['directory']+=1

    return file_types

def plot_histogram(file_types):
    gnuplot_commands = []
    gnuplot_commands.append("set terminal jpeg")
    gnuplot_commands.append("set output 'histogram.jpg'")
    gnuplot_commands.append("set title 'File Type Histogram'")
    gnuplot_commands.append("set xlabel 'File Type'")
    gnuplot_commands.append("set ylabel 'Frequency'")
    gnuplot_commands.append("set style data histogram")
    gnuplot_commands.append("set style fill solid")
    gnuplot_commands.append("set xtics rotate by -45")
    gnuplot_commands.append("plot '-' using 2:xtic(1) with histogram")
    for file_type, frequency in file_types.items():
        gnuplot_commands.append("{} {}".format(file_type, frequency))
    gnuplot_commands.append("e")

    gnuplot_process = subprocess.Popen(['gnuplot'], stdin=subprocess.PIPE)
    gnuplot_process.communicate("\n".join(gnuplot_commands).encode())

def main():
    directory = sys.argv[1]
    file_types = generate_histogram(directory)
    plot_histogram(file_types)

if __name__ == "__main__":
    main()
