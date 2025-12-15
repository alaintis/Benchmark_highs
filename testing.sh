#!/bin/bash
set -e  # exit immediately on error

# -------- Configuration --------
BUILD_DIR=build2
PROBLEM_DIR=../../dphpc-simplex-data/netlib/csc/presolved/
HIGHS_DIR=$HOME/highs-install/lib/cmake/highs
ACCOUNT=dphpc
TIME=00:10
# --------------------------------

echo "Cleaning build directory..."
rm -rf ${BUILD_DIR}

echo "Creating build directory..."
mkdir ${BUILD_DIR}
cd ${BUILD_DIR}

echo "Configuring with CMake..."
cmake .. -Dhighs_DIR=${HIGHS_DIR}

echo "Building..."
make -j

echo "Running solver with srun..."
srun -A ${ACCOUNT} -t ${TIME} ./solve ${PROBLEM_DIR}

echo "Done."
#!/bin/bash
set -euo pipefail

###############################################################################
# HiGHS Batch Test Runner (Robust, Continue-on-Failure)
###############################################################################

DATA_DIR="../../dphpc-simplex-data/netlib/csc/presolved/selection_problems"
BUILD_DIR="build2"
EXEC="$BUILD_DIR/solve"

ACCOUNT="dphpc"
SRUN_TIME="00:10"
TIMEOUT="20s"

RESULTS_DIR="results"
mkdir -p "$RESULTS_DIR"
LOGFILE="$RESULTS_DIR/full_log_highs.txt"
OUTFILE="$RESULTS_DIR/results_highs.txt"

MAX=100   # max number of problems (set large if you want all)



###############################################################################
# Build
###############################################################################

echo "Cleaning build directory..."
rm -rf "$BUILD_DIR"
mkdir "$BUILD_DIR"
cd "$BUILD_DIR"

echo "Configuring..."
cmake .. -Dhighs_DIR=$HOME/highs-install/lib/cmake/highs

echo "Building..."
make -j

if [ ! -x "$EXEC" ]; then
    echo "ERROR: executable $EXEC not found"
    exit 1
fi

cd ..

###############################################################################
# Run batch
###############################################################################

> "$OUTFILE"
> "$LOGFILE"

count=0

echo "---------------------------------------------"
echo "Running HiGHS batch tests"
echo "Data dir:     $DATA_DIR"
echo "Executable:   $EXEC"
echo "Timeout:      $TIMEOUT"
echo "---------------------------------------------"

for file in "$DATA_DIR"/*.csc; do
    ((count++))

    if [ $count -gt $MAX ]; then
        echo "Stopping early after $MAX problems."
        break
    fi

    echo "[$count] Running: $(basename "$file")"

    OUTPUT=$(timeout "$TIMEOUT" \
        srun -A "$ACCOUNT" -t "$SRUN_TIME" \
        "$EXEC" "$file" 2>&1)
    EXITCODE=$?

    {
        echo "============ $file ============"
        echo "$OUTPUT"
        echo ""
    } >> "$LOGFILE"

    # ---- timeout
    if [ $EXITCODE -eq 124 ]; then
        echo "  -> TIMEOUT"
        continue
    fi

    # ---- crash / solver failure
    if [ $EXITCODE -ne 0 ]; then
        echo "  -> FAILED (exit code $EXITCODE)"
        continue
    fi

    # ---- optional START/END block handling
    BLOCK=$(echo "$OUTPUT" | sed -n '/^START$/,/^END$/p')
    if [[ -z "$BLOCK" ]]; then
        echo "  -> FAILED (no START/END block)"
        continue
    fi

    CLEANED=$(echo "$BLOCK" | sed '/^START$/d;/^END$/d')

    {
        echo "$file"
        echo "$CLEANED"
        echo ""
    } >> "$OUTFILE"

    echo "  -> OK"
done

echo "---------------------------------------------"
echo "Completed."
echo "Results:   $OUTFILE"
echo "Logs:      $LOGFILE"
echo "Processed: $count problems"
