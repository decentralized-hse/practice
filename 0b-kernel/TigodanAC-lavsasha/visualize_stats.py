import json
import matplotlib.pyplot as plt
import matplotlib.dates as mdates
from datetime import datetime


def main():
    with open('rust_kernel_stats.json', 'r', encoding='utf-8') as f:
        data = json.load(f)

    dates = []
    files = []
    lines = []

    for date_str, stats in data.items():
        dates.append(datetime.strptime(date_str, "%Y-%m-%d"))
        files.append(stats['files'])
        lines.append(stats['lines'])

    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12, 10))

    ax1.plot(dates, files, 'o-', linewidth=2, markersize=8, color='royalblue')
    ax1.set_title('Количество Rust файлов в ядре Linux по годам', fontsize=14, fontweight='bold')
    ax1.set_ylabel('Количество файлов', fontsize=12)
    ax1.grid(True, alpha=0.3)

    for i, (date, file_count) in enumerate(zip(dates, files)):
        ax1.annotate(f'{file_count}', 
                    (date, file_count),
                    textcoords="offset points",
                    xytext=(0,10),
                    ha='center',
                    fontsize=9)

    ax2.plot(dates, lines, 's-', linewidth=2, markersize=8, color='crimson')
    ax2.set_title('Количество строк Rust кода в ядре Linux по годам', fontsize=14, fontweight='bold')
    ax2.set_ylabel('Количество строк', fontsize=12)
    ax2.set_xlabel('Дата', fontsize=12)
    ax2.grid(True, alpha=0.3)

    for ax in [ax1, ax2]:
        ax.xaxis.set_major_formatter(mdates.DateFormatter('%Y-%m-%d'))
        ax.xaxis.set_major_locator(mdates.YearLocator())
        fig.autofmt_xdate()

    for i, (date, line_count) in enumerate(zip(dates, lines)):
        ax2.annotate(f'{line_count:,}', 
                    (date, line_count),
                    textcoords="offset points",
                    xytext=(0,10),
                    ha='center',
                    fontsize=9)

    fig.suptitle('Статистика использования Rust в ядре Linux\n', fontsize=16, fontweight='bold')

    plt.tight_layout()
    plt.savefig('rust_kernel_stats.png', dpi=300, bbox_inches='tight')
    plt.show()  

if __name__ == "__main__":
    main()