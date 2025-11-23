#!/bin/bash
# Local binary statistics generator
# This script generates the same stats as the CI workflow for local testing

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}=== Homeshell Binary Statistics Generator ===${NC}"
echo ""

# Check if binary exists
if [ ! -f "build/homeshell-linux" ]; then
    echo -e "${RED}Error: build/homeshell-linux not found${NC}"
    echo "Please build the project first:"
    echo "  mkdir -p build && cd build && cmake .. && make -j\$(nproc)"
    exit 1
fi

# Create stats directory
mkdir -p stats
echo -e "${GREEN}✓ Created stats directory${NC}"

# Get file sizes
BINARY="build/homeshell-linux"
BINARY_SIZE=$(stat -c%s "$BINARY")
BINARY_SIZE_MB=$(echo "scale=2; $BINARY_SIZE / 1048576" | bc)

echo -e "${BLUE}Binary Size:${NC} $BINARY_SIZE_MB MB"

# Strip and get stripped size
cp "$BINARY" "$BINARY-stripped"
strip "$BINARY-stripped"
STRIPPED_SIZE=$(stat -c%s "$BINARY-stripped")
STRIPPED_SIZE_MB=$(echo "scale=2; $STRIPPED_SIZE / 1048576" | bc)

echo -e "${BLUE}Stripped Size:${NC} $STRIPPED_SIZE_MB MB"

# Get symbol count
SYMBOL_COUNT=$(nm -C "$BINARY" 2>/dev/null | wc -l || echo "0")
echo -e "${BLUE}Symbol Count:${NC} $SYMBOL_COUNT"

# Get dynamic linkage info
echo -e "\n${YELLOW}=== Dynamic Library Dependencies ===${NC}"
ldd "$BINARY" 2>&1 | tee stats/ldd.txt || echo "Static binary (no dynamic dependencies)" | tee stats/ldd.txt

# Get ELF info
echo -e "\n${YELLOW}=== ELF Information ===${NC}"
readelf -h "$BINARY" | tee stats/readelf.txt
echo "" >> stats/readelf.txt
echo "=== Section Headers ===" >> stats/readelf.txt
readelf -S "$BINARY" >> stats/readelf.txt

# Get section sizes
echo -e "\n${YELLOW}=== Section Sizes ===${NC}"
size "$BINARY" | tee stats/sections.txt
echo "" >> stats/sections.txt
echo "Detailed Section Breakdown:" >> stats/sections.txt
objdump -h "$BINARY" | grep -E '^\s+[0-9]+ \.' | tee -a stats/sections.txt

# Get top symbols
echo -e "\n${YELLOW}=== Top 20 Symbols by Size ===${NC}"
echo "Top 50 Symbols by Size:" > stats/top-symbols.txt
nm -C -S --size-sort "$BINARY" 2>/dev/null | tail -n 50 | tac >> stats/top-symbols.txt || echo "Symbol size information not available" >> stats/top-symbols.txt
head -n 20 stats/top-symbols.txt | tail -n 20

# Get composition
echo -e "\n${YELLOW}=== Symbol Composition ===${NC}"
echo "Symbol Distribution by Namespace:" > stats/composition.txt
nm -C "$BINARY" 2>/dev/null | grep -E ' [TtWw] ' | sed 's/.*\s\w\s//' | cut -d'(' -f1 | cut -d':' -f1 | cut -d'<' -f1 | sort | uniq -c | sort -rn | head -n 20 >> stats/composition.txt || echo "Composition analysis not available" >> stats/composition.txt
cat stats/composition.txt

# Summary
echo ""
echo -e "${GREEN}=== Summary ===${NC}"
echo -e "Binary Size:    ${BINARY_SIZE_MB} MB"
echo -e "Stripped Size:  ${STRIPPED_SIZE_MB} MB"
echo -e "Symbols:        ${SYMBOL_COUNT}"
echo -e "Output:         ${PWD}/stats/"

# Cleanup
rm -f "$BINARY-stripped"

echo ""
echo -e "${GREEN}✓ Statistics generated successfully!${NC}"
echo -e "View detailed stats in: ${BLUE}stats/${NC}"
echo ""
echo "Files created:"
echo "  - stats/ldd.txt         (library dependencies)"
echo "  - stats/readelf.txt     (ELF information)"
echo "  - stats/sections.txt    (section sizes)"
echo "  - stats/top-symbols.txt (largest symbols)"
echo "  - stats/composition.txt (symbol distribution)"

