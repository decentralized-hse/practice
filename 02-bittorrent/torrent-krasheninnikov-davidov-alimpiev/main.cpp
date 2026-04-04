#include <array>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <libtorrent/session.hpp>
#include <libtorrent/add_torrent_params.hpp>
#include <libtorrent/torrent_handle.hpp>
#include <libtorrent/torrent_status.hpp>
#include <libtorrent/peer_info.hpp>
#include <libtorrent/sha1_hash.hpp>
#include <libtorrent/session_params.hpp>
#include <libtorrent/settings_pack.hpp>
#include <boost/algorithm/hex.hpp>


namespace lt = libtorrent;

struct TorrentEntry {
  std::string name;
  std::string info_hash_hex;
};

struct SwarmStats {
  std::string name;
  std::string info_hash;
  int total_peers{0};
  int tcp_peers{0};
  int utp_peers{0};
  double ledbat_pct{0.0};
};

static const std::vector<std::vector<std::string>> TRACKERS = {
  {"udp://tracker.opentrackr.org:1337/announce", "udp://open.stealth.si:80/announce"},
  {"udp://tracker.torrent.eu.org:451/announce", "http://tracker.opentrackr.org:1337/announce"},
};

static const std::vector<TorrentEntry> TORRENTS = {
  {"Debian 12 Bookworm", "176de2e6724976a38d51d0a34d3c0503a2305e63"},
  {"Arch Linux 2026.03", "a4373c326657898d0c588c3ff892a0fac97ffa20"},
  {"Linux Mint 21.3 Cinnamon", "14bb29461c2243aa287cda681488e57c9eeb25af"},
  {"openSUSE Leap 15.5", "1b70100e267620455a2bf8d804301b860b2e4514"},
  {"Ubuntu Server 25.10", "ccbd47a30a5a13a5260295e4bd65c038244e9df0"},
  {"Fedora KDE Desktop Live 43 x86", "3290eb9c2c0884baf0afefbf052eaa9f33d7c828"},
  {"Kali Linux 2025.4 Live arm64", "4627a04f80dd09ae4d7c14db73edfe190cad4e84"},
  {"GitGud Code Dataset", "221571632238b826f0aa6ec4f370af633575cae4"},
  {"NIST National Vulnerability Database", "fe623a0bbd13e8f152ea2317f151d8d3719ba96b"},
  {"English Wikipedia Dump (pre-chatgpt)", "ab13148ab9b64f11c9548fb87bf05f8ce64cb15a"},
  {"NOAA Global Surface Temperature", "f98189ad498d773acd831b590dd8b1612cf6111d"},
  {"JihuLab Code Dataset", "004c9565d325cf8951aa23d3a56ebb806898fc7f"},
  {"The Oxford-IIIT Pet Dataset", "b18bbd9ba03d50b0f7f479acc9f4228a408cecc1"},
  {"grok-1", "5f96d43576e3d386c9ba65b883210a393b68210e"},
  {"Bitcoin: A Peer-to-Peer Electronic Cash System", "8c271f4d2e92a3449e2d1bde633cd49f64af888f"},
};

static constexpr int WAIT_SECONDS_PER_TORRENT = 300;
static constexpr int SLEEP_INTERVAL_SEC = 10;
static const std::string RESULTS_DIR = "results";
static const std::string CSV_PATH = "results/ledbat_analysis.csv";

std::string get_date_string() {
  auto t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  std::ostringstream os;
  os << std::put_time(std::gmtime(&t), "%Y-%m-%d");
  return os.str();
}

bool sha1_from_hex(const std::string& hex, lt::sha1_hash& out) {
  if (hex.size() != 40) return false;
  try {
    std::string buf;
    buf.reserve(20);
    boost::algorithm::unhex(hex.begin(), hex.end(), std::back_inserter(buf));
    if (buf.size() != 20) return false;
    out.assign(buf.data());
    return true;
  } catch (...) {
    return false;
  }
}

lt::session make_session() {
  lt::settings_pack pack;
  pack.set_int(lt::settings_pack::alert_mask, lt::alert_category::status);
  pack.set_str(lt::settings_pack::listen_interfaces, "0.0.0.0:0,[::]:0");
  pack.set_bool(lt::settings_pack::enable_dht, true);
  pack.set_bool(lt::settings_pack::enable_lsd, false);
  pack.set_bool(lt::settings_pack::enable_upnp, false);
  pack.set_bool(lt::settings_pack::enable_natpmp, false);
  pack.set_int(lt::settings_pack::connections_limit, 200);
  pack.set_bool(lt::settings_pack::enable_outgoing_tcp, true);
  pack.set_bool(lt::settings_pack::enable_incoming_tcp, true);
  pack.set_bool(lt::settings_pack::enable_outgoing_utp, true);
  pack.set_bool(lt::settings_pack::enable_incoming_utp, true);
  return lt::session(lt::session_params(pack));
}

lt::add_torrent_params make_add_torrent_params(const TorrentEntry& entry, const lt::sha1_hash& hash) {
  lt::add_torrent_params atp;
  atp.info_hashes = lt::info_hash_t(hash);
  atp.save_path = ".";
  for (const auto& tier : TRACKERS)
    for (const auto& url : tier)
      atp.trackers.push_back(url);
  return atp;
}

void wait_for_peers(int seconds) {
  for (int elapsed = 0; elapsed < seconds; elapsed += SLEEP_INTERVAL_SEC)
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_INTERVAL_SEC));
}

std::pair<int, int> get_peer_counts(lt::torrent_handle h) {
  int tcp = 0, utp = 0;
  if (!h.is_valid()) return {tcp, utp};
  std::vector<lt::peer_info> peers;
  h.get_peer_info(peers);
  for (const auto& p : peers) {
    if (p.flags & lt::peer_info::utp_socket)
      utp++;
    else
      tcp++;
  }
  return {tcp, utp};
}

std::optional<SwarmStats> analyze_torrent(lt::session& ses, const TorrentEntry& t) {
  lt::sha1_hash hash;
  if (!sha1_from_hex(t.info_hash_hex, hash)) {
    std::cerr << "Invalid info hash: " << t.info_hash_hex << " (" << t.name << ")\n";
    return std::nullopt;
  }

  lt::torrent_handle h;
  try {
    h = ses.add_torrent(make_add_torrent_params(t, hash));
    h.resume();
  } catch (const std::exception& e) {
    std::cerr << "Failed to add " << t.name << ": " << e.what() << "\n";
    return std::nullopt;
  }

  std::cout << "  Waiting " << WAIT_SECONDS_PER_TORRENT << " s for " << t.name << "... ";
  std::cout.flush();
  wait_for_peers(WAIT_SECONDS_PER_TORRENT);

  auto [tcp, utp] = get_peer_counts(h);
  ses.remove_torrent(h);

  int total = tcp + utp;
  double pct = (total > 0) ? (100.0 * utp / total) : 0.0;
  std::cout << total << " peers (" << tcp << " TCP, " << utp << " µTP)\n";

  SwarmStats s;
  s.name = t.name;
  s.info_hash = t.info_hash_hex;
  s.total_peers = total;
  s.tcp_peers = tcp;
  s.utp_peers = utp;
  s.ledbat_pct = pct;
  return s;
}

void ensure_results_dir(const std::string& dir) {
  system(("mkdir -p " + dir).c_str());
}

void write_csv(const std::string& path, const std::vector<SwarmStats>& stats,
               const std::string& analysis_date) {
  std::ofstream csv(path);
  if (!csv.is_open()) return;
  csv << "torrent_name,info_hash,total_peers,tcp_peers,utp_peers,ledbat_percentage,analysis_date\n";
  for (const auto& s : stats) {
    csv << "\"" << s.name << "\"," << s.info_hash << ","
        << s.total_peers << "," << s.tcp_peers << "," << s.utp_peers << ","
        << std::fixed << std::setprecision(2) << s.ledbat_pct << ","
        << analysis_date << "\n";
  }
  std::cout << "  " << path << "\n";
}

int main(int argc, char* argv[]) {
  (void)argc;
  (void)argv;
  const auto& torrents = TORRENTS;
  std::cout << torrents.size() << " torrents\n\n";

  auto ses = make_session();
  std::string analysis_date = get_date_string();
  std::vector<SwarmStats> stats;

  for (const auto& t : torrents) {
    auto s = analyze_torrent(ses, t);
    if (s)
      stats.push_back(std::move(*s));
  }

  ses.pause();
  std::cout << "\nWriting results...\n";
  ensure_results_dir(RESULTS_DIR);
  write_csv(CSV_PATH, stats, analysis_date);
  std::cout << "\nDone.\n";
  return 0;
}
