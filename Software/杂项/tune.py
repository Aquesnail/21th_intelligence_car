#!/usr/bin/env python3
"""
Transpose all notes in music.h melody array by a given number of semitones.
Uses pitch definitions from pitches.h to determine the chromatic order.

File paths are hardcoded:
- input_music  = "music.h"
- input_pitches = "pitches.h"
- output_file  = "music_transposed.h"
"""

import re
import sys

# ========== 硬编码文件路径 ==========
INPUT_MUSIC = "music.h"
INPUT_PITCHES = "pitches.h"
OUTPUT_FILE = "Sources/Apptest/music.h"
# ===================================

def parse_pitches(pitches_path):
    """
    Read pitches.h and return an ordered list of note names (without 'NOTE_' prefix)
    in chromatic order.
    """
    notes = []
    with open(pitches_path, 'r', encoding='utf-8') as f:
        for line in f:
            line = line.strip()
            if line.startswith('#define NOTE_'):
                parts = line.split()
                if len(parts) >= 2:
                    macro = parts[1]          # e.g. NOTE_B0
                    note_name = macro[5:]     # remove 'NOTE_' prefix
                    notes.append(note_name)
    if not notes:
        raise ValueError(f"No note definitions found in {pitches_path}")
    return notes

def note_name_to_index(notes):
    """Create a dict mapping note name -> index in the chromatic list."""
    return {name: idx for idx, name in enumerate(notes)}

def transpose_note(match, notes, name_to_idx, shift):
    """
    Callback for re.sub: replace a matched note macro with its transposed version.
    match.group(0) is the full macro, e.g. 'NOTE_FS3'.
    """
    full_macro = match.group(0)
    note_name = full_macro[5:]  # strip 'NOTE_'
    if note_name not in name_to_idx:
        print(f"Warning: {full_macro} not found in pitch list, keeping unchanged.", file=sys.stderr)
        return full_macro

    old_idx = name_to_idx[note_name]
    new_idx = old_idx + shift
    if new_idx < 0 or new_idx >= len(notes):
        print(f"Warning: {full_macro} transposed out of range (index {new_idx}), keeping unchanged.", file=sys.stderr)
        return full_macro

    new_note = notes[new_idx]
    return f"NOTE_{new_note}"

def transpose_music(music_path, pitches_path, shift, output_path):
    """
    Read music.h, transpose the melody array, and write the result to output_path.
    """
    notes = parse_pitches(pitches_path)
    name_to_idx = note_name_to_index(notes)

    note_re = re.compile(r'NOTE_[A-G]S?\d+')

    inside_melody = False
    modified_lines = []

    with open(music_path, 'r', encoding='utf-8') as f:
        for line in f:
            if not inside_melody and 'melody[]' in line and '=' in line and '{' in line:
                inside_melody = True
                modified_lines.append(line)
                continue

            if inside_melody and '};' in line:
                inside_melody = False
                modified_lines.append(line)
                continue

            if inside_melody:
                new_line = note_re.sub(
                    lambda m: transpose_note(m, notes, name_to_idx, shift),
                    line
                )
                modified_lines.append(new_line)
            else:
                modified_lines.append(line)

    with open(output_path, 'w', encoding='utf-8') as f:
        f.writelines(modified_lines)

    print(f"Transposed by {shift} semitones. Output written to {output_path}")

def main():
    try:
        shift_input = input("请输入升降半音数（正数升调，负数降调）: ").strip()
        if not shift_input:
            print("未输入，退出。")
            return
        shift = int(shift_input)
    except ValueError:
        print("输入无效，请输入一个整数。")
        sys.exit(1)

    try:
        transpose_music(INPUT_MUSIC, INPUT_PITCHES, shift, OUTPUT_FILE)
    except Exception as e:
        print(f"错误: {e}", file=sys.stderr)
        sys.exit(1)

if __name__ == '__main__':
    main()