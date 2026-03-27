import pandas as pd
import sys
import os
import argparse

# Simple script:
# - read a CSV with 2 columns
# - prepend row [0.0, 400.0]
# - transpose so result has two rows
# - save to output CSV

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("input_file", help="Input CSV file")
    parser.add_argument("output_file", help="Output CSV file")
    args = parser.parse_args()

    input_file = args.input_file
    output_file = args.output_file

    # Read CSV (no header expected)
    df = pd.read_csv(input_file, header=None)

    # Ensure at least two columns; take first two if more
    if df.shape[1] < 2:
        print("Error: input CSV must have at least 2 columns.")
        sys.exit(1)
    df = df.iloc[:, :2]

    # Replace NaN values with -1
    df = df.fillna(-1)

    # Prepend the row [0.0, 400.0]
    prepended = pd.concat([pd.DataFrame([[0.0, 400.0]]), df], ignore_index=True)

    # Transpose so result has two rows
    result = prepended.transpose()

    # Ensure output directory exists
    outdir = os.path.dirname(output_file)
    if outdir:
        os.makedirs(outdir, exist_ok=True)

    # Save without header and without index
    result.to_csv(output_file, header=False, index=False)
    print(f"Wrote {output_file}")

if __name__ == "__main__":
    main()


