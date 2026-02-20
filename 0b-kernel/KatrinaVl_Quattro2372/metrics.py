import os
import json
import glob
import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
from collections import Counter

INPUT_DIR = "trending_repos_commits_metrics"
OUTPUT_DIR = "visualizations"
DAYS_ORDER = ["Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"]

sns.set_theme(style="whitegrid")
plt.rcParams.update({'figure.max_open_warning': 0})

def ensure_dir(path):
    if not os.path.exists(path):
        os.makedirs(path)

def get_commit_ratio(row):
    added = row['lines_added']
    deleted = row['lines_deleted']
    if added > 0:
        return deleted / added
    return 0.0

def process_dataframe(df):
    df['weekday'] = pd.Categorical(df['weekday'], categories=DAYS_ORDER, ordered=True)
    df['churn_ratio'] = df.apply(get_commit_ratio, axis=1)
    return df

def plot_repo_metrics(df, repo_name, language, output_path):
    fig = plt.figure(figsize=(20, 15))
    gs = fig.add_gridspec(3, 2)
    fig.suptitle(f"Repository: {repo_name} ({language})", fontsize=20, weight='bold')

    ax1 = fig.add_subplot(gs[0, 0])
    avg_size = df['commit_size'].mean()
    sns.histplot(df['commit_size'], kde=True, ax=ax1, color='skyblue', bins=30)
    ax1.axvline(avg_size, color='red', linestyle='--', label=f'Mean: {avg_size:.0f}')
    ax1.set_title("1. Commit Size Distribution")
    ax1.legend()

    ax2 = fig.add_subplot(gs[0, 1])
    heatmap_data = pd.crosstab(df['weekday'], df['hour'])
    heatmap_data = heatmap_data.reindex(index=DAYS_ORDER, columns=range(24), fill_value=0)
    sns.heatmap(heatmap_data, cmap="YlGnBu", ax=ax2)
    ax2.set_title("2. Activity Heatmap")

    ax3 = fig.add_subplot(gs[1, 0])
    top_authors = df['author'].value_counts().head(10)
    sns.barplot(x=top_authors.values, y=top_authors.index, ax=ax3, palette="viridis", hue=top_authors.index, legend=False)
    ax3.set_title("3. Top Authors")

    ax4 = fig.add_subplot(gs[1, 1])
    avg_add = df['lines_added'].mean()
    avg_del = df['lines_deleted'].mean()
    avg_ratio = df['churn_ratio'].mean()
    
    stats_df = pd.DataFrame({
        'Metric': ['Avg Added', 'Avg Deleted'],
        'Value': [avg_add, avg_del]
    })
    
    sns.barplot(data=stats_df, x='Metric', y='Value', ax=ax4, palette=["green", "red"], hue='Metric', legend=False)
    ax4.set_title(f"4. Churn Stats (Avg Ratio per commit: {avg_ratio:.2f})")

    ax5 = fig.add_subplot(gs[2, 0])
    avg_files = df['files_count'].mean()
    q95 = df['files_count'].quantile(0.95)
    filtered = df[df['files_count'] <= (q95 + 5)]
    sns.histplot(filtered['files_count'], kde=False, discrete=True, ax=ax5, color='orange')
    ax5.axvline(avg_files, color='blue', linestyle='--', label=f'Mean: {avg_files:.1f}')
    ax5.set_title("5. Changed Files per Commit")
    ax5.legend()

    ax6 = fig.add_subplot(gs[2, 1])
    all_files = [f for sublist in df['files'] for f in sublist]
    if all_files:
        common_files = Counter(all_files).most_common(5)
        x_val = [count for _, count in common_files]
        y_val = [(name if len(name) < 40 else "..."+name[-37:]) for name, _ in common_files]
        sns.barplot(x=x_val, y=y_val, ax=ax6, palette="rocket", hue=y_val, legend=False)
        ax6.set_title("6. Top Modified Files")
    
    plt.tight_layout()
    plt.savefig(output_path)
    plt.close(fig)

def plot_language_summary(df, language, output_path):
    fig = plt.figure(figsize=(16, 12))
    gs = fig.add_gridspec(2, 2)
    fig.suptitle(f"Language: {language.upper()}", fontsize=22, weight='bold')

    ax1 = fig.add_subplot(gs[0, 0])
    avg_size = df['commit_size'].mean()
    q95 = df['commit_size'].quantile(0.95)
    data_filtered = df[df['commit_size'] <= q95]
    sns.histplot(data=data_filtered, x='commit_size', kde=True, ax=ax1, color='teal', bins=30)
    ax1.axvline(avg_size, color='red', linestyle='--', label=f'Mean: {avg_size:.0f}')
    ax1.set_title("1. Commit Size Distribution")
    ax1.legend()

    ax2 = fig.add_subplot(gs[0, 1])
    heatmap_data = pd.crosstab(df['weekday'], df['hour'])
    heatmap_data = heatmap_data.reindex(index=DAYS_ORDER, columns=range(24), fill_value=0)
    sns.heatmap(heatmap_data, cmap="coolwarm", ax=ax2)
    ax2.set_title("2. Activity Heatmap")

    ax3 = fig.add_subplot(gs[1, 0])
    avg_add = df['lines_added'].mean()
    avg_del = df['lines_deleted'].mean()
    avg_ratio = df['churn_ratio'].mean()
    
    stats_df = pd.DataFrame({
        'Metric': ['Avg Added', 'Avg Deleted'],
        'Value': [avg_add, avg_del]
    })
    sns.barplot(data=stats_df, x='Metric', y='Value', ax=ax3, palette="pastel", hue='Metric', legend=False)
    ax3.set_title(f"3. Churn Stats (Avg Ratio per commit: {avg_ratio:.2f})")

    ax4 = fig.add_subplot(gs[1, 1])
    avg_files = df['files_count'].mean()
    q99 = df['files_count'].quantile(0.99)
    files_filtered = df[df['files_count'] <= q99]
    sns.histplot(data=files_filtered, x='files_count', discrete=True, ax=ax4, color='purple')
    ax4.axvline(avg_files, color='orange', linestyle='--', label=f'Mean: {avg_files:.1f}')
    ax4.set_title("4. Files Changed per Commit")
    ax4.legend()

    plt.tight_layout()
    plt.savefig(output_path)
    plt.close(fig)

def plot_global_comparison(stats_list, output_path):
    if not stats_list:
        return

    df = pd.DataFrame(stats_list)
    fig, axes = plt.subplots(2, 2, figsize=(18, 12))
    fig.suptitle("Global Comparison Across Languages", fontsize=24, weight='bold')

    sns.barplot(data=df, x='language', y='avg_commit_size', ax=axes[0, 0], palette='viridis', hue='language', legend=False)
    axes[0, 0].set_title("Average Commit Size")
    axes[0, 0].tick_params(axis='x', rotation=45)

    sns.barplot(data=df, x='language', y='avg_files_count', ax=axes[0, 1], palette='magma', hue='language', legend=False)
    axes[0, 1].set_title("Average Files Changed")
    axes[0, 1].tick_params(axis='x', rotation=45)

    sns.barplot(data=df, x='language', y='avg_churn_ratio', ax=axes[1, 0], palette='coolwarm', hue='language', legend=False)
    axes[1, 0].set_title("Average Delete/Add Ratio")
    axes[1, 0].tick_params(axis='x', rotation=45)

    sns.barplot(data=df, x='language', y='total_commits', ax=axes[1, 1], palette='cubehelix', hue='language', legend=False)
    axes[1, 1].set_title("Total Commits Analyzed")
    axes[1, 1].tick_params(axis='x', rotation=45)

    plt.tight_layout()
    plt.savefig(output_path)
    plt.close(fig)

def main():
    if not os.path.exists(INPUT_DIR):
        print(f"Directory {INPUT_DIR} not found.")
        return

    ensure_dir(OUTPUT_DIR)
    agg_dir = os.path.join(OUTPUT_DIR, "_language_summaries")
    ensure_dir(agg_dir)

    lang_dirs = glob.glob(os.path.join(INPUT_DIR, "*"))
    global_stats = []

    for lang_path in lang_dirs:
        if not os.path.isdir(lang_path):
            continue
            
        language = os.path.basename(lang_path)
        print(f"Processing: {language}")
        
        lang_out_dir = os.path.join(OUTPUT_DIR, language)
        ensure_dir(lang_out_dir)

        all_commits = []
        json_files = glob.glob(os.path.join(lang_path, "*.json"))
        
        for json_file in json_files:
            try:
                with open(json_file, 'r', encoding='utf-8') as f:
                    data = json.load(f)
                
                commits = data.get('commits', [])
                if not commits:
                    continue
                
                df_repo = pd.DataFrame(commits)
                df_repo = process_dataframe(df_repo)
                
                repo_name = data.get('repo_name', os.path.splitext(os.path.basename(json_file))[0])
                out_file = os.path.join(lang_out_dir, f"{repo_name}.png")
                
                plot_repo_metrics(df_repo, repo_name, language, out_file)
                
                all_commits.extend(commits)

            except Exception as e:
                print(f"Error {json_file}: {e}")

        if all_commits:
            df_lang = pd.DataFrame(all_commits)
            df_lang = process_dataframe(df_lang)
            
            summary_file = os.path.join(agg_dir, f"{language}_summary.png")
            plot_language_summary(df_lang, language, summary_file)

            global_stats.append({
                'language': language,
                'avg_commit_size': df_lang['commit_size'].mean(),
                'avg_files_count': df_lang['files_count'].mean(),
                'avg_churn_ratio': df_lang['churn_ratio'].mean(),
                'total_commits': len(df_lang)
            })

    if global_stats:
        global_file = os.path.join(OUTPUT_DIR, "global_languages_comparison.png")
        plot_global_comparison(global_stats, global_file)
        print(f"Global comparison saved to {global_file}")

if __name__ == "__main__":
    main()
