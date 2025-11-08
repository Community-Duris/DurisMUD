#!/bin/bash
#
# This script is for me to import the help to my duris server in my home lab. make sure to be in duris root folder (the one with startmud.sh)
# DurisMUD Help Import Script.
# Imports all help content to production server:
#   1. Individual help files -> mud_info + pages tables
#   2. Help index entries -> pages table
#   3. Parsed help file entries -> pages table
#
# Usage:
#   ./import_help_to_prod.sh [OPTIONS]
#
# Options:
#   --local              Import to localhost MySQL directly (default)
#   --remote <host>      Import to remote server via SSH (implies --ssh mode)
#   --user <username>    SSH username for remote connection (default: current user)
#   --clean              Clear all existing help entries before import
#   --dry-run            Show what would be imported without making changes
#
# Examples:
#   ./import_help_to_prod.sh --dry-run
#   ./import_help_to_prod.sh --local
#   ./import_help_to_prod.sh --clean --dry-run
#   ./import_help_to_prod.sh --clean
#   ./import_help_to_prod.sh --remote 192.168.1.100
#   ./import_help_to_prod.sh --remote myserver.com --user admin
#   ./import_help_to_prod.sh --remote 10.0.0.5 --user duris --dry-run

set -e

# ============================================================================
# CONFIGURATION
# ============================================================================
REMOTE_HOST=""          # Will be set by --remote argument
REMOTE_USER="$USER"     # Default to current user
MYSQL_USER="duris"      # MySQL username
MYSQL_PASS="duris"      # MySQL password
MYSQL_DB="duris_dev"    # Database name

HELP_DIR="lib/information"
HELP_INDEX_FILE="lib/information/help_index"
PARSED_HELP_FILE="duris_help_parsed.hlp"

# ============================================================================
# PARSE ARGUMENTS
# ============================================================================
DRY_RUN=0
USE_SSH=0  # Default to local mode
CLEAN_DB=0  # Default to not cleaning database

while [[ $# -gt 0 ]]; do
    case "$1" in
        --dry-run)
            DRY_RUN=1
            shift
            ;;
        --clean)
            CLEAN_DB=1
            shift
            ;;
        --remote)
            if [ -z "$2" ] || [[ "$2" == --* ]]; then
                echo "Error: --remote requires a hostname or IP address"
                exit 1
            fi
            REMOTE_HOST="$2"
            USE_SSH=1
            shift 2
            ;;
        --user)
            if [ -z "$2" ] || [[ "$2" == --* ]]; then
                echo "Error: --user requires a username"
                exit 1
            fi
            REMOTE_USER="$2"
            shift 2
            ;;
        --local)
            USE_SSH=0
            shift
            ;;
        --help|-h)
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  --local              Import to localhost MySQL directly (default)"
            echo "  --remote <host>      Import to remote server via SSH"
            echo "  --user <username>    SSH username for remote (default: $USER)"
            echo "  --clean              Clear all existing help entries before import"
            echo "  --dry-run            Show what would be imported without changes"
            echo "  --help, -h           Show this help message"
            echo ""
            echo "Examples:"
            echo "  $0 --dry-run"
            echo "  $0 --local"
            echo "  $0 --clean --dry-run"
            echo "  $0 --clean"
            echo "  $0 --remote 192.168.1.100"
            echo "  $0 --remote myserver.com --user admin"
            echo "  $0 --remote 10.0.0.5 --user duris --dry-run"
            exit 0
            ;;
        *)
            echo "Unknown argument: $1"
            echo "Usage: $0 [--local|--remote <host>] [--user <username>] [--clean] [--dry-run]"
            echo "Use --help for more information"
            exit 1
            ;;
    esac
done

# Validate SSH mode requirements
if [ $USE_SSH -eq 1 ] && [ -z "$REMOTE_HOST" ]; then
    echo "Error: SSH mode requires --remote <host>"
    exit 1
fi

# ============================================================================
# HELPER FUNCTIONS
# ============================================================================

# Execute SQL query based on mode (local or SSH)
execute_sql() {
    local sql="$1"
    if [ $USE_SSH -eq 1 ]; then
        # SSH mode: execute on remote server
        echo "$sql" | ssh "$REMOTE_USER@$REMOTE_HOST" "mysql -u$MYSQL_USER -p$MYSQL_PASS $MYSQL_DB"
    else
        # Local mode: execute directly (use -h127.0.0.1 for containerized MySQL)
        echo "$sql" | mysql -h127.0.0.1 -u$MYSQL_USER -p$MYSQL_PASS $MYSQL_DB
    fi
}

# Execute SQL file based on mode (local or SSH)
execute_sql_file() {
    local sqlfile="$1"
    if [ $USE_SSH -eq 1 ]; then
        # SSH mode: copy to remote and execute
        scp -q "$sqlfile" "$REMOTE_USER@$REMOTE_HOST:/tmp/import_sql.tmp"
        ssh "$REMOTE_USER@$REMOTE_HOST" "mysql -u$MYSQL_USER -p$MYSQL_PASS $MYSQL_DB < /tmp/import_sql.tmp"
        ssh "$REMOTE_USER@$REMOTE_HOST" "rm /tmp/import_sql.tmp"
    else
        # Local mode: execute directly (use -h127.0.0.1 for containerized MySQL)
        mysql -h127.0.0.1 -u$MYSQL_USER -p$MYSQL_PASS $MYSQL_DB < "$sqlfile"
    fi
}

# ============================================================================
# HEADER
# ============================================================================
echo "=== DurisMUD Unified Help Import to Production ===="
if [ $USE_SSH -eq 1 ]; then
    echo "Mode: SSH (Remote)"
    echo "Remote Host: $REMOTE_HOST"
else
    echo "Mode: LOCAL"
    echo "Host: 127.0.0.1"
fi
echo "Database: $MYSQL_DB"
echo ""
echo "This will import:"
echo "  1. Individual help files (mud_info + pages)"
echo "  2. Help index entries (~535 entries)"
echo "  3. Parsed help file entries"
echo ""

if [ $DRY_RUN -eq 1 ]; then
    echo "Mode: DRY RUN (no changes will be made)"
else
    echo "Mode: LIVE (changes will be committed to production)"
    echo ""
    read -p "Continue with production import? (yes/no): " confirm
    if [ "$confirm" != "yes" ]; then
        echo "Aborted."
        exit 0
    fi
fi
echo ""

# ============================================================================
# TEST CONNECTION
# ============================================================================
if [ $DRY_RUN -eq 0 ]; then
    echo "Testing connection to database..."
    if ! execute_sql "SELECT 1;" 2>/dev/null >/dev/null; then
        echo "ERROR: Cannot connect to database!"
        if [ $USE_SSH -eq 1 ]; then
            echo "Check SSH connection to $REMOTE_HOST and MySQL credentials"
        else
            echo "Check MySQL credentials and that MySQL server is running"
        fi
        exit 1
    fi
    echo "Connected successfully!"
    echo ""
fi

# ============================================================================
# CLEAN DATABASE (if --clean flag is used)
# ============================================================================
if [ $CLEAN_DB -eq 1 ]; then
    echo "=== Cleaning Database ==="
    echo ""

    if [ $DRY_RUN -eq 1 ]; then
        echo "WOULD CLEAN:"
        echo "  - TRUNCATE TABLE pages (all help entries)"
        echo "  - DELETE FROM mud_info WHERE name IN ('news', 'motd', 'wizmotd', 'credits')"
        echo ""
    else
        echo "WARNING: This will DELETE ALL existing help entries!"
        read -p "Are you sure you want to continue? (yes/no): " clean_confirm
        if [ "$clean_confirm" != "yes" ]; then
            echo "Aborted."
            exit 0
        fi

        echo "Cleaning pages table..."
        if execute_sql "TRUNCATE TABLE pages;" 2>&1; then
            echo "  ✓ pages table cleared"
        else
            echo "  ERROR: Failed to truncate pages table"
            exit 1
        fi

        echo "Cleaning mud_info entries..."
        if execute_sql "DELETE FROM mud_info WHERE name IN ('news', 'motd', 'wizmotd', 'credits');" 2>&1; then
            echo "  ✓ mud_info entries cleared"
        else
            echo "  ERROR: Failed to clear mud_info entries"
            exit 1
        fi

        echo ""
        echo "Database cleaned successfully!"
        echo ""
    fi
fi

# ============================================================================
# SECTION 1: IMPORT INDIVIDUAL HELP FILES
# ============================================================================
echo "=== SECTION 1: Importing Individual Help Files ==="
echo ""

# Files to import to mud_info table
declare -A MUD_INFO_FILES=(
    ["motd"]="motd"
    ["news"]="news"
    ["wizmotd"]="wizmotd"
)

# Files to import to pages table
declare -A HELP_FILES=(
    ["help"]="help"
    ["help.1"]="help commands"
    ["help.2"]="help advanced"
    ["helpguild1"]="guilds"
    ["helpguild2"]="guild commands"
    ["helpships"]="ships"
    ["helpkingdoms"]="kingdoms"
    ["faq"]="faq"
    ["rules"]="rules"
    ["info"]="info"
    ["credits"]="credits"
    ["wizlist"]="wizlist"
    ["hints.txt"]="hints"
)

# Import mud_info files
echo ">> Importing to mud_info table..."
for filename in "${!MUD_INFO_FILES[@]}"; do
    name="${MUD_INFO_FILES[$filename]}"
    filepath="$HELP_DIR/$filename"

    if [ ! -f "$filepath" ]; then
        echo "  SKIP: $filename (file not found)"
        continue
    fi

    tmpfile=$(mktemp)

    # Create SQL with hex encoding
    {
        echo -n "REPLACE INTO mud_info (name, content) VALUES ('$name', 0x"
        xxd -p "$filepath" | tr -d '\n'
        echo ");"
    } > "$tmpfile"

    if [ $DRY_RUN -eq 1 ]; then
        echo "  WOULD IMPORT: $name ($(wc -c < "$filepath") bytes)"
    else
        if execute_sql_file "$tmpfile" 2>&1; then
            echo "  IMPORTED: $name ($(wc -c < "$filepath") bytes)"
        else
            echo "  ERROR importing $name"
        fi
    fi

    rm "$tmpfile"
done

echo ""
echo ">> Importing to pages table..."
for filename in "${!HELP_FILES[@]}"; do
    title="${HELP_FILES[$filename]}"
    filepath="$HELP_DIR/$filename"

    if [ ! -f "$filepath" ]; then
        echo "  SKIP: $filename (file not found)"
        continue
    fi

    tmpfile=$(mktemp)
    now=$(date '+%Y-%m-%d %H:%M:%S')

    # Use hex encoding for content with DELETE then INSERT
    {
        echo "DELETE FROM pages WHERE title = '$title';"
        echo -n "INSERT INTO pages (title, text, last_update, last_update_by, category_id) VALUES ('$title', 0x"
        xxd -p "$filepath" | tr -d '\n'
        echo ", '$now', 'Arih_importDB', 0);"
    } > "$tmpfile"

    if [ $DRY_RUN -eq 1 ]; then
        echo "  WOULD IMPORT: '$title' from $filename ($(wc -c < "$filepath") bytes)"
    else
        if execute_sql_file "$tmpfile" 2>&1; then
            echo "  IMPORTED: '$title' from $filename ($(wc -c < "$filepath") bytes)"
        else
            echo "  ERROR importing '$title'"
        fi
    fi

    rm "$tmpfile"
done

echo ""

# ============================================================================
# SECTION 2: IMPORT HELP_INDEX ENTRIES
# ============================================================================
echo "=== SECTION 2: Importing Help Index Entries ==="
echo ""

if [ ! -f "$HELP_INDEX_FILE" ]; then
    echo "WARNING: $HELP_INDEX_FILE not found, skipping..."
else
    # Parse help_index using Python
    echo "Parsing help_index file..."
    python3 << PYTHON_SCRIPT > /tmp/help_index_entries.txt
import re

def parse_help_index(filename):
    with open(filename, 'r', encoding='utf-8', errors='ignore') as f:
        content = f.read()

    entries = content.split('\\n#\\n')
    help_entries = []

    for entry in entries:
        entry = entry.strip()
        if not entry or entry.startswith('last update:'):
            continue

        lines = entry.split('\\n')
        if not lines:
            continue

        title_line = lines[0].strip()

        # Try with quotes first
        match = re.match(r'^"([^"]+)"', title_line)
        if match:
            title = match.group(1).strip()
        else:
            # Try without quotes - title is everything before first parenthesis or end of line
            title = title_line.split('(')[0].strip()
            if not title:
                continue

        content_lines = lines[1:]
        content = '\\n'.join(content_lines).strip()
        content = re.sub(r'^=+\\n', '', content)
        content = re.sub(r'\\n=+$', '', content)
        content = content.strip()

        if title and content:
            # Output format: title|length
            print(f"{title}|{len(content)}")

entries = parse_help_index('$HELP_INDEX_FILE')
PYTHON_SCRIPT

    entry_count=$(wc -l < /tmp/help_index_entries.txt)
    echo "Found $entry_count help_index entries"
    echo ""

    if [ $DRY_RUN -eq 1 ]; then
        echo "First 20 entries:"
        head -20 /tmp/help_index_entries.txt | while IFS='|' read -r title length; do
            echo "  - '$title' ($length bytes)"
        done
        if [ $entry_count -gt 20 ]; then
            echo "  ... and $((entry_count - 20)) more"
        fi
    else
        echo "Importing help_index entries..."
        # Import using Python
        python3 << PYTHON_SCRIPT
import re
import subprocess
import sys
from datetime import datetime

USE_SSH = $USE_SSH
REMOTE_HOST = "$REMOTE_HOST"
REMOTE_USER = "$REMOTE_USER"
MYSQL_USER = "$MYSQL_USER"
MYSQL_PASS = "$MYSQL_PASS"
MYSQL_DB = "$MYSQL_DB"

def parse_help_index(filename):
    with open(filename, 'r', encoding='utf-8', errors='ignore') as f:
        content = f.read()

    entries = content.split('\\n#\\n')
    help_entries = []

    for entry in entries:
        entry = entry.strip()
        if not entry or entry.startswith('last update:'):
            continue

        lines = entry.split('\\n')
        if not lines:
            continue

        title_line = lines[0].strip()

        # Try with quotes first
        match = re.match(r'^"([^"]+)"', title_line)
        if match:
            title = match.group(1).strip()
        else:
            # Try without quotes - title is everything before first parenthesis or end of line
            title = title_line.split('(')[0].strip()
            if not title:
                continue

        content_lines = lines[1:]
        content = '\\n'.join(content_lines).strip()
        content = re.sub(r'^=+\\n', '', content)
        content = re.sub(r'\\n=+\$', '', content)
        content = content.strip()

        if title and content:
            help_entries.append((title, content))

    return help_entries

entries = parse_help_index('$HELP_INDEX_FILE')
now = datetime.now().strftime('%Y-%m-%d %H:%M:%S')

success_count = 0
error_count = 0

for title, content in entries:
    # Create SQL with hex encoding
    content_hex = content.encode('utf-8').hex()

    sql = f"""DELETE FROM pages WHERE title = '{title.replace("'", "''")}';
INSERT INTO pages (title, text, last_update, last_update_by, category_id)
VALUES ('{title.replace("'", "''")}', 0x{content_hex}, '{now}', 'Arih_importDB', 0);"""

    # Write SQL to temp file
    with open('/tmp/import_help_entry.sql', 'w') as f:
        f.write(sql)

    # Execute based on mode (SSH or local)
    if USE_SSH == 1:
        # SSH mode: copy and execute remotely
        scp_result = subprocess.run(
            ['scp', '-q', '/tmp/import_help_entry.sql', f'{REMOTE_USER}@{REMOTE_HOST}:/tmp/import_help_entry.sql'],
            capture_output=True
        )

        if scp_result.returncode != 0:
            print(f"  ERROR uploading SQL for '{title}'", file=sys.stderr)
            error_count += 1
            continue

        mysql_result = subprocess.run(
            ['ssh', f'{REMOTE_USER}@{REMOTE_HOST}',
             f'mysql -u{MYSQL_USER} -p{MYSQL_PASS} {MYSQL_DB} < /tmp/import_help_entry.sql'],
            capture_output=True, text=True
        )
    else:
        # Local mode: execute directly (use -h127.0.0.1 for containerized MySQL)
        mysql_result = subprocess.run(
            ['mysql', '-h127.0.0.1', f'-u{MYSQL_USER}', f'-p{MYSQL_PASS}', MYSQL_DB],
            stdin=open('/tmp/import_help_entry.sql', 'r'),
            capture_output=True, text=True
        )

    if mysql_result.returncode == 0:
        success_count += 1
        if success_count % 50 == 0:
            print(f"  Imported {success_count}/{len(entries)} entries...")
    else:
        print(f"  ERROR importing '{title}': {mysql_result.stderr}", file=sys.stderr)
        error_count += 1

print(f"")
print(f"Help Index Import Complete:")
print(f"  Success: {success_count}")
print(f"  Errors: {error_count}")
PYTHON_SCRIPT
    fi

    rm -f /tmp/help_index_entries.txt
fi

echo ""

# ============================================================================
# SECTION 3: IMPORT PARSED HELP FILE
# ============================================================================
echo "=== SECTION 3: Importing Parsed Help File ==="
echo ""

if [ ! -f "$PARSED_HELP_FILE" ]; then
    echo "WARNING: $PARSED_HELP_FILE not found, skipping..."
else
    # Parse and import using Python
    if [ $DRY_RUN -eq 1 ]; then
        python3 << PYTHON_SCRIPT
import re
import sys

HELP_FILE = "$PARSED_HELP_FILE"

def parse_parsed_help(filename):
    """Parse duris_help_parsed.hlp file which uses #0 as separator."""
    with open(filename, 'r', encoding='utf-8', errors='ignore') as f:
        content = f.read()

    # Split by #0 marker
    entries = content.split('\\n#0\\n')
    help_entries = []

    for entry in entries:
        entry = entry.strip()
        if not entry:
            continue

        lines = entry.split('\\n')
        if len(lines) < 2:
            continue

        # Second line has the full title with metadata (first line is often truncated)
        # Format: "Full Title - Last Edited: YYYY-MM-DD HH:MM:SS by user"
        title_line = lines[1].strip()

        # Skip if it looks like continuation of previous entry
        if not title_line or title_line.startswith('==') or title_line.startswith('*'):
            continue

        # Title is everything before " - Last Edited:"
        title = title_line.split(' - Last Edited:')[0].strip()

        # Skip empty titles
        if not title:
            continue

        # Content is all remaining lines
        content_lines = lines[1:]
        content = '\\n'.join(content_lines).strip()

        if title and content and len(content) > 10:
            help_entries.append((title, content))

    return help_entries

entries = parse_parsed_help(HELP_FILE)
print(f"Parsed {len(entries)} help entries")
print(f"")
print(f"First 20 entries:")
for i, (title, content) in enumerate(entries[:20]):
    print(f"  {i+1}. '{title}' ({len(content)} bytes)")
if len(entries) > 20:
    print(f"  ... and {len(entries) - 20} more")
PYTHON_SCRIPT
    else
        python3 << PYTHON_SCRIPT
import re
import subprocess
import sys
from datetime import datetime

USE_SSH = $USE_SSH
REMOTE_HOST = "$REMOTE_HOST"
REMOTE_USER = "$REMOTE_USER"
MYSQL_USER = "$MYSQL_USER"
MYSQL_PASS = "$MYSQL_PASS"
MYSQL_DB = "$MYSQL_DB"
HELP_FILE = "$PARSED_HELP_FILE"

def parse_parsed_help(filename):
    """Parse duris_help_parsed.hlp file which uses #0 as separator."""
    with open(filename, 'r', encoding='utf-8', errors='ignore') as f:
        content = f.read()

    # Split by #0 marker
    entries = content.split('\\n#0\\n')
    help_entries = []

    for entry in entries:
        entry = entry.strip()
        if not entry:
            continue

        lines = entry.split('\\n')
        if len(lines) < 2:
            continue

        # Second line has the full title with metadata (first line is often truncated)
        # Format: "Full Title - Last Edited: YYYY-MM-DD HH:MM:SS by user"
        title_line = lines[1].strip()

        # Skip if it looks like continuation of previous entry
        if not title_line or title_line.startswith('==') or title_line.startswith('*'):
            continue

        # Title is everything before " - Last Edited:"
        title = title_line.split(' - Last Edited:')[0].strip()

        # Skip empty titles
        if not title:
            continue

        # Content is all remaining lines
        content_lines = lines[1:]
        content = '\\n'.join(content_lines).strip()

        if title and content and len(content) > 10:
            help_entries.append((title, content))

    return help_entries

entries = parse_parsed_help(HELP_FILE)
print(f"Parsed {len(entries)} help entries")

now = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
success_count = 0
error_count = 0

for title, content in entries:
    # Create SQL with hex encoding
    content_hex = content.encode('utf-8').hex()

    # Escape single quotes in title
    safe_title = title.replace("'", "''")

    sql = f"""DELETE FROM pages WHERE title = '{safe_title}';
INSERT INTO pages (title, text, last_update, last_update_by, category_id)
VALUES ('{safe_title}', 0x{content_hex}, '{now}', 'Arih_importDB', 0);"""

    # Write SQL to temp file
    with open('/tmp/import_help_entry.sql', 'w') as f:
        f.write(sql)

    # Execute based on mode (SSH or local)
    if USE_SSH == 1:
        # SSH mode: copy and execute remotely
        scp_result = subprocess.run(
            ['scp', '-q', '/tmp/import_help_entry.sql', f'{REMOTE_USER}@{REMOTE_HOST}:/tmp/import_help_entry.sql'],
            capture_output=True
        )

        if scp_result.returncode != 0:
            print(f"  ERROR uploading SQL for '{title}'", file=sys.stderr)
            error_count += 1
            continue

        mysql_result = subprocess.run(
            ['ssh', f'{REMOTE_USER}@{REMOTE_HOST}',
             f'mysql -u{MYSQL_USER} -p{MYSQL_PASS} {MYSQL_DB} < /tmp/import_help_entry.sql'],
            capture_output=True, text=True
        )
    else:
        # Local mode: execute directly (use -h127.0.0.1 for containerized MySQL)
        mysql_result = subprocess.run(
            ['mysql', '-h127.0.0.1', f'-u{MYSQL_USER}', f'-p{MYSQL_PASS}', MYSQL_DB],
            stdin=open('/tmp/import_help_entry.sql', 'r'),
            capture_output=True, text=True
        )

    if mysql_result.returncode == 0:
        success_count += 1
        if success_count % 50 == 0:
            print(f"  Imported {success_count}/{len(entries)} entries...")
    else:
        print(f"  ERROR importing '{title}': {mysql_result.stderr}", file=sys.stderr)
        error_count += 1

print(f"")
print(f"Parsed Help Import Complete:")
print(f"  Success: {success_count}")
print(f"  Errors: {error_count}")
PYTHON_SCRIPT
    fi
fi

# ============================================================================
# CLEANUP
# ============================================================================
rm -f /tmp/import_help_entry.sql

echo ""
if [ $DRY_RUN -eq 1 ]; then
    echo "=== Dry run complete. Run without --dry-run to apply to production ==="
else
    echo "=== All Import Operations Complete! ==="
fi
