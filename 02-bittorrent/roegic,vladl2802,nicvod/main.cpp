#include <libtorrent/session.hpp>
#include <libtorrent/session_params.hpp>
#include <libtorrent/magnet_uri.hpp>
#include <libtorrent/torrent_handle.hpp>
#include <libtorrent/torrent_info.hpp>
#include <libtorrent/torrent_status.hpp>
#include <libtorrent/peer_info.hpp>
#include <libtorrent/alert_types.hpp>
#include <libtorrent/alert.hpp>
#include <libtorrent/settings_pack.hpp>

#include <iostream>
#include <iomanip>
#include <string>
#include <set>
#include <map>
#include <vector>
#include <numeric>
#include <thread>
#include <chrono>
#include <algorithm>
#include <sstream>

namespace lt = libtorrent;
using namespace std::chrono_literals;



void drain_alerts(lt::session& ses) {
    std::vector<lt::alert*> alerts;
    ses.pop_alerts(&alerts);
}

std::string format_speed(int64_t bps) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(1);
    if      (bps < 1024)        { ss << bps             << " B/s";  }
    else if (bps < 1024*1024)   { ss << bps/1024.0      << " KB/s"; }
    else                        { ss << bps/1048576.0   << " MB/s"; }
    return ss.str();
}

std::string format_size(int64_t b) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2);
    if      (b < 1024)              { ss << b                  << " B";  }
    else if (b < 1024*1024)         { ss << b/1024.0           << " KB"; }
    else if (b < 1024LL*1024*1024)  { ss << b/1048576.0        << " MB"; }
    else                            { ss << b/1073741824.0     << " GB"; }
    return ss.str();
}

void print_bar(const std::string& label, int value, int max_val, int width = 30) {
    int filled = (max_val > 0) ? (width * value / max_val) : 0;
    std::string bar(filled, '#');
    bar += std::string(width - filled, '.');
    std::cout << "  " << std::left  << std::setw(14) << label
              << " [" << bar << "] " << value << "\n";
}

lt::session create_session() {
    lt::settings_pack pack;

    pack.set_bool(lt::settings_pack::enable_dht,    true);
    pack.set_bool(lt::settings_pack::enable_lsd,    true);
    pack.set_bool(lt::settings_pack::enable_upnp,   true);
    pack.set_bool(lt::settings_pack::enable_natpmp, true);

    pack.set_str(lt::settings_pack::listen_interfaces, "0.0.0.0:6881");
    pack.set_str(lt::settings_pack::dht_bootstrap_nodes,
        "router.bittorrent.com:6881,"
        "router.utorrent.com:6881,"
        "dht.transmissionbt.com:6881,"
        "dht.aelitis.com:6881"
    );

    pack.set_int(lt::settings_pack::request_timeout,      10);
    pack.set_int(lt::settings_pack::peer_connect_timeout, 10);
    pack.set_int(lt::settings_pack::connection_speed,     50);
    pack.set_int(lt::settings_pack::connections_limit,    500);
    pack.set_int(lt::settings_pack::active_limit,         20);
    pack.set_int(lt::settings_pack::active_downloads,     10);
    pack.set_int(lt::settings_pack::active_seeds,         10);

    return lt::session(pack);
}


void wait_for_dht(lt::session& ses, int seconds = 15) {
    std::cout << "Bootstrapping DHT...\n";
    std::vector<lt::alert*> alerts;

    for (int i = 0; i < seconds; ++i) {
        ses.pop_alerts(&alerts);
        alerts.clear();

        std::this_thread::sleep_for(1s);
        std::cout << "  " << (i + 1) << "s elapsed\r" << std::flush;
    }
    std::cout << "\nDHT bootstrap complete\n";
}

void observe_torrent(lt::session& ses,
                     const std::string& magnet,
                     int observe_seconds = 60)
{
    lt::error_code ec;
    lt::add_torrent_params params = lt::parse_magnet_uri(magnet, ec);
    if (ec) {
        std::cerr << "Bad magnet: " << ec.message() << "\n";
        return;
    }

    params.save_path = "/tmp";
    params.flags     = lt::torrent_flags::default_flags;

    lt::torrent_handle handle = ses.add_torrent(params);

    std::cout << "Torrent added\n";
    std::cout << "Info-hash: " << handle.info_hashes().v1 << "\n";
    std::cout << "Waiting for metadata...\n\n";

    std::vector<lt::alert*> alerts;
    bool got_metadata = false;

    for (int i = 0; i < 120; ++i) {
        auto status = handle.status();

        ses.pop_alerts(&alerts);
        alerts.clear();

        std::cout << "  " << (i + 1) << "s"
                  << " | peers: " << status.num_peers
                  << "\r" << std::flush;

        if (status.has_metadata) {
            got_metadata = true;
            std::cout << "\n\nMetadata received!\n";
            break;
        }

        std::this_thread::sleep_for(1s);
    }

    if (!got_metadata) {
        std::cout << "\n\nTimeout - metadata not received\n";
        return;
    }

    // statistics
    std::set<std::string>       peers_seen;
    std::map<std::string, int>  clients_counter;
    std::vector<int64_t>        dl_samples, ul_samples;
    std::vector<int>            peer_counts, seed_counts;
    int64_t peak_dl    = 0, peak_ul = 0;
    int     peak_peers = 0;

    std::cout << "Observing swarm for " << observe_seconds << "s...\n\n";

    for (int tick = 0; tick < observe_seconds; ++tick) {
        auto    status = handle.status();
        int64_t dl     = status.download_rate;
        int64_t ul     = status.upload_rate;

        peak_dl    = std::max(peak_dl,    dl);
        peak_ul    = std::max(peak_ul,    ul);
        peak_peers = std::max(peak_peers, status.num_peers);

        dl_samples.push_back(dl);
        ul_samples.push_back(ul);
        peer_counts.push_back(status.num_peers);
        seed_counts.push_back(status.num_seeds);

        // пиры
        std::vector<lt::peer_info> peers;
        handle.get_peer_info(peers);

        for (auto& p : peers) {
            peers_seen.insert(p.ip.address().to_string());
            std::string client = p.client.empty() ? "Unknown" : p.client;
            clients_counter[client]++;
        }

        ses.pop_alerts(&alerts);
        alerts.clear();

        std::cout << "  " << std::setw(3) << (tick + 1) << "s"
                  << " | peers: " << std::setw(4) << status.num_peers
                  << " | seeds: " << std::setw(4) << status.num_seeds
                  << " | down: " << std::setw(12) << format_speed(dl)
                  << " | up: "   << std::setw(12) << format_speed(ul)
                  << "\r" << std::flush;

        std::this_thread::sleep_for(1s);
    }

    auto tf = handle.torrent_file();

    auto avg_fn = [](const auto& v) -> double {
        if (v.empty()) return 0;
        return std::accumulate(v.begin(), v.end(), 0LL) / (double)v.size();
    };

    double avg_dl    = avg_fn(dl_samples);
    double avg_ul    = avg_fn(ul_samples);
    double avg_peers = avg_fn(peer_counts);
    double avg_seeds = avg_fn(seed_counts);

    int total_obs = 0;
    for (auto& [k, v] : clients_counter) total_obs += v;

    std::vector<std::pair<std::string,int>> clients_sorted(
        clients_counter.begin(), clients_counter.end()
    );
    std::sort(clients_sorted.begin(), clients_sorted.end(),
        [](auto& a, auto& b){ return a.second > b.second; });

    std::cout << "\n\n";
    std::cout << "============================================================\n";
    std::cout << "         TORRENT STATISTICS REPORT\n";
    std::cout << "============================================================\n";

    std::cout << "\n  Torrent Info\n";
    std::cout << "  Name    : " << tf->name()                    << "\n";
    std::cout << "  Size    : " << format_size(tf->total_size()) << "\n";
    std::cout << "  Files   : " << tf->num_files()               << "\n";
    std::cout << "  Infohash: " << handle.info_hashes().v1       << "\n";

    std::cout << "\n  Download Speed\n";
    std::cout << "  Average : " << format_speed((int64_t)avg_dl) << "\n";
    std::cout << "  Peak    : " << format_speed(peak_dl)         << "\n";
    std::cout << "  Samples : " << dl_samples.size() << " sec\n";

    std::cout << "\n  Upload Speed\n";
    std::cout << "  Average : " << format_speed((int64_t)avg_ul) << "\n";
    std::cout << "  Peak    : " << format_speed(peak_ul)         << "\n";

    std::cout << "\n  Swarm Population\n";
    std::cout << "  Unique IPs  : " << peers_seen.size() << "\n";
    std::cout << "  Avg peers   : " << std::fixed << std::setprecision(1)
              << avg_peers << "\n";
    std::cout << "  Peak peers  : " << peak_peers << "\n";
    std::cout << "  Avg seeds   : " << avg_seeds  << "\n";

    std::cout << "\n  Peers over time:\n";
    int max_p = *std::max_element(peer_counts.begin(), peer_counts.end());
    int step  = std::max(1, (int)peer_counts.size() / 10);
    for (int i = 0; i < (int)peer_counts.size(); i += step) {
        print_bar("t=" + std::to_string(i) + "s", peer_counts[i], max_p);
    }

    std::cout << "\n  Clients (total observations: " << total_obs << ")\n";
    std::cout << "  " << std::left  << std::setw(35) << "Client"
              << std::right << std::setw(6) << "Count"
              << std::setw(8) << "Share"
              << "  Bar\n";
    std::cout << "  " << std::string(72, '-') << "\n";

    int show      = std::min((int)clients_sorted.size(), 15);
    int max_count = clients_sorted.empty() ? 1 : clients_sorted[0].second;

    for (int i = 0; i < show; ++i) {
        auto& [client, count] = clients_sorted[i];
        double share = total_obs > 0 ? count * 100.0 / total_obs : 0;
        int    bar_l = (int)(20.0 * count / max_count);
        std::string bar(bar_l, '#');
        bar += std::string(20 - bar_l, '.');

        std::cout << "  " << std::left  << std::setw(35) << client
                  << std::right << std::setw(6) << count
                  << std::setw(7) << std::fixed << std::setprecision(1)
                  << share << "%  " << bar << "\n";
    }

    if ((int)clients_sorted.size() > 15) {
        int others = 0;
        for (int i = 15; i < (int)clients_sorted.size(); ++i)
            others += clients_sorted[i].second;
        double share = total_obs > 0 ? others * 100.0 / total_obs : 0;
        std::cout << "  " << std::left  << std::setw(35) << "... others"
                  << std::right << std::setw(6) << others
                  << std::setw(7) << std::fixed << std::setprecision(1)
                  << share << "%\n";
    }

    std::cout << "\n============================================================\n";
}

int main() {
    lt::session ses = create_session();
    wait_for_dht(ses, 15);

    std::string magnet;
    std::cout << "Enter magnet link: ";
    std::getline(std::cin, magnet);

    if (magnet.empty()) {
        std::cerr << "Magnet link is empty\n";
        return 1;
    }

    observe_torrent(ses, magnet, 60);
    return 0;
}
