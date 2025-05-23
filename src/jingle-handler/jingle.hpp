#pragma once
#include <span>
#include <unordered_map>

#include <coop/single-event.hpp>

#include "../codec-type.hpp"
#include "../xmpp/extdisco.hpp"
#include "../xmpp/jid.hpp"
#include "ice.hpp"

struct Codec {
    CodecType type;
    int       tx_pt;
    int       rtx_pt = -1;

    std::vector<jingle::RTCPFeedBack> rtcp_fbs;
};

constexpr auto rtp_hdrext_ssrc_audio_level_uri = "urn:ietf:params:rtp-hdrext:ssrc-audio-level";
constexpr auto rtp_hdrext_transport_cc_uri     = "http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01";

enum class SourceType {
    Audio,
    Video,
};

struct Source {
    uint32_t    ssrc;
    SourceType  type;
    std::string participant_id;
};

using SSRCMap = std::unordered_map<uint32_t, Source>;

struct JingleSession {
    jingle::Jingle       initiate_jingle;
    ice::Agent           ice_agent;
    ice::LocalCredential local_cred;
    std::string          fingerprint_str;
    std::string          dtls_cert_pem;
    std::string          dtls_priv_key_pem;
    std::vector<Codec>   codecs;
    SSRCMap              ssrc_map;
    uint32_t             audio_ssrc;
    uint32_t             video_ssrc;
    uint32_t             video_rtx_ssrc;
    int                  video_hdrext_transport_cc     = -1;
    int                  audio_hdrext_transport_cc     = -1;
    int                  audio_hdrext_ssrc_audio_level = -1;

    auto find_codec_by_type(CodecType type) const -> const Codec*;
    auto find_codec_by_tx_pt(int tx_pt) const -> const Codec*;
};

class JingleHandler {
  private:
    coop::SingleEvent*             sync;
    CodecType                      audio_codec_type;
    CodecType                      video_codec_type;
    xmpp::Jid                      jid;
    std::span<const xmpp::Service> external_services;
    JingleSession                  session;

  public:
    auto get_session() const -> const JingleSession&;
    auto build_accept_jingle() const -> std::optional<jingle::Jingle>;
    auto on_initiate(jingle::Jingle jingle) -> bool;
    auto on_add_source(jingle::Jingle jingle) -> bool;

    JingleHandler(CodecType                      audio_codec_type,
                  CodecType                      video_codec_type,
                  xmpp::Jid                      jid,
                  std::span<const xmpp::Service> external_services,
                  coop::SingleEvent*             sync);
};
