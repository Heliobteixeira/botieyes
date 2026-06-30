#!/bin/bash
# Check postponed features and technical debt

echo "╔════════════════════════════════════════════════════════════════╗"
echo "║              POSTPONED FEATURES & TECHNICAL DEBT               ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""

echo "📋 INCOMPLETE TASKS:"
echo "─────────────────────────────────────────────────────────────────"
grep -n "^- \[ \]" specs/**/tasks.md | while IFS=: read -r file line content; do
    task_id=$(echo "$content" | grep -o 'T[0-9]\+' | head -1)
    echo "  $task_id: $(basename $(dirname $file))"
    echo "    $content"
    echo ""
done

echo ""
echo "🔧 CODE TODOs:"
echo "─────────────────────────────────────────────────────────────────"
find esp-idf/components esp-idf/main -type f \( -name "*.c" -o -name "*.cpp" -o -name "*.h" \) \
    -exec grep -Hn "TODO\|FIXME\|XXX" {} \; 2>/dev/null | head -20

echo ""
echo "⚠️  TEMPORARY COMPONENTS:"
echo "─────────────────────────────────────────────────────────────────"
find esp-idf/components -name "README.md" -exec sh -c '
    if grep -qi "temporary\|TEMPORARY\|workaround\|WORKAROUND" "$1"; then
        echo "  $(dirname $1 | xargs basename)"
        grep -i "temporary\|workaround" "$1" | head -2 | sed "s/^/    /"
        echo ""
    fi
' _ {} \;

echo ""
echo "📊 SUMMARY:"
echo "─────────────────────────────────────────────────────────────────"
incomplete_tasks=$(grep -c "^- \[ \]" specs/**/tasks.md 2>/dev/null || echo 0)
todo_count=$(find esp-idf/components esp-idf/main -type f \( -name "*.c" -o -name "*.cpp" -o -name "*.h" \) -exec grep -c "TODO\|FIXME\|XXX" {} \; 2>/dev/null | awk '{s+=$1} END {print s}')

echo "  Incomplete tasks: $incomplete_tasks"
echo "  TODO comments: $todo_count"
echo ""
echo "📖 For details, see: docs/postponed-features.md"
echo ""
