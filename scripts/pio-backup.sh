#!/bin/bash
# PlatformIO 完整环境备份与恢复脚本
# 用法:
#   ./pio-backup.sh backup [备份名称]  - 备份整个 PlatformIO 环境
#   ./pio-backup.sh restore [备份名称] - 从备份恢复
#   ./pio-backup.sh list               - 列出所有备份
#   ./pio-backup.sh clean [保留数量]   - 清理旧备份，默认保留3个

set -e

# 配置
BACKUP_DIR="$HOME/pio_backup"
PIO_DIR="$HOME/.platformio"

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

log_info() { echo -e "${GREEN}[INFO]${NC} $1"; }
log_warn() { echo -e "${YELLOW}[WARN]${NC} $1"; }
log_error() { echo -e "${RED}[ERROR]${NC} $1"; }

mkdir -p "$BACKUP_DIR"

# 备份函数
do_backup() {
    local name="${1:-$(date +%Y%m%d_%H%M%S)}"
    local backup_file="$BACKUP_DIR/platformio-$name.tar.gz"

    if [ -f "$backup_file" ]; then
        log_error "备份 '$name' 已存在: $backup_file"
        exit 1
    fi

    if [ ! -d "$PIO_DIR" ]; then
        log_error "PlatformIO 目录不存在: $PIO_DIR"
        exit 1
    fi

    log_info "开始备份 PlatformIO 完整环境..."
    log_info "源目录: $PIO_DIR"
    log_info "备份文件: $backup_file"

    # 显示目录结构
    echo ""
    log_info "包含的内容:"
    du -sh "$PIO_DIR"/* 2>/dev/null | while read size dir; do
        echo "  $size  $(basename "$dir")"
    done
    echo ""

    # 创建压缩备份
    tar -czf "$backup_file" -C "$HOME" .platformio

    local size=$(du -h "$backup_file" | cut -f1)
    log_info "备份完成! 文件大小: $size"
    log_info "备份位置: $backup_file"
}

# 恢复函数
do_restore() {
    local name="$1"
    local backup_file=""

    if [ -z "$name" ]; then
        backup_file=$(ls -t "$BACKUP_DIR"/platformio-*.tar.gz 2>/dev/null | head -1)
        if [ -z "$backup_file" ]; then
            log_error "没有找到任何备份文件"
            exit 1
        fi
        log_info "使用最新备份: $(basename "$backup_file")"
    else
        backup_file="$BACKUP_DIR/platformio-$name.tar.gz"
        if [ ! -f "$backup_file" ]; then
            log_error "备份文件不存在: $backup_file"
            log_info "使用 'list' 命令查看可用备份"
            exit 1
        fi
    fi

    log_warn "即将恢复 PlatformIO 完整环境"
    log_warn "这将覆盖: $PIO_DIR"
    read -p "确认继续? (y/N) " confirm
    if [ "$confirm" != "y" ] && [ "$confirm" != "Y" ]; then
        log_info "已取消"
        exit 0
    fi

    log_info "开始恢复..."

    # 备份当前状态
    if [ -d "$PIO_DIR" ]; then
        local temp_backup="$BACKUP_DIR/platformio-pre-restore-$(date +%Y%m%d_%H%M%S).tar.gz"
        log_info "备份当前状态到: $(basename "$temp_backup")"
        tar -czf "$temp_backup" -C "$HOME" .platformio 2>/dev/null || true
    fi

    # 清空并恢复
    rm -rf "$PIO_DIR"

    log_info "解压备份文件..."
    tar -xzf "$backup_file" -C "$HOME"

    log_info "恢复完成!"
    du -sh "$PIO_DIR"/* 2>/dev/null | while read size dir; do
        echo "  $size  $(basename "$dir")"
    done
}

# 列出备份
do_list() {
    log_info "可用的备份:"
    echo ""

    local count=0
    for f in "$BACKUP_DIR"/platformio-*.tar.gz; do
        if [ -f "$f" ]; then
            local name=$(basename "$f" | sed 's/platformio-//' | sed 's/.tar.gz//')
            local size=$(du -h "$f" | cut -f1)
            local date=$(stat -f "%Sm" -t "%Y-%m-%d %H:%M" "$f" 2>/dev/null || stat -c "%y" "$f" 2>/dev/null | cut -d' ' -f1-2)
            printf "  %-25s %8s  %s\n" "$name" "$size" "$date"
            count=$((count + 1))
        fi
    done

    if [ $count -eq 0 ]; then
        echo "  (无备份)"
    fi
    echo ""
    log_info "备份目录: $BACKUP_DIR"
}

# 清理旧备份
do_clean() {
    local keep="${1:-3}"

    log_info "清理旧备份，保留最新 $keep 个..."

    local files=$(ls -t "$BACKUP_DIR"/platformio-*.tar.gz 2>/dev/null | tail -n +$((keep + 1)))

    if [ -z "$files" ]; then
        log_info "没有需要清理的备份"
        return
    fi

    echo "$files" | while read f; do
        log_info "删除: $(basename "$f")"
        rm -f "$f"
    done

    log_info "清理完成"
}

# 显示帮助
show_help() {
    echo "PlatformIO 完整环境备份与恢复脚本"
    echo ""
    echo "用法: $0 <命令> [参数]"
    echo ""
    echo "命令:"
    echo "  backup [名称]     备份整个 PlatformIO 环境（默认使用时间戳命名）"
    echo "  restore [名称]    从备份恢复（默认使用最新备份）"
    echo "  list              列出所有备份"
    echo "  clean [数量]      清理旧备份，保留指定数量（默认3个）"
    echo ""
    echo "示例:"
    echo "  $0 backup stable          # 创建名为 'stable' 的备份"
    echo "  $0 restore stable         # 从 'stable' 备份恢复"
    echo "  $0 restore                # 从最新备份恢复"
    echo "  $0 list                   # 查看所有备份"
    echo ""
    echo "备份目录: $BACKUP_DIR"
    echo "PlatformIO 目录: $PIO_DIR"
}

case "${1:-help}" in
    backup)  do_backup "$2" ;;
    restore) do_restore "$2" ;;
    list)    do_list ;;
    clean)   do_clean "$2" ;;
    help|--help|-h) show_help ;;
    *)
        log_error "未知命令: $1"
        show_help
        exit 1
        ;;
esac
