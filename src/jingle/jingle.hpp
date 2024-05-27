#pragma once
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "../xml/xml.hpp"

namespace jingle {
template <bool optional_value>
struct Parameter {
    std::string name;
    std::string value;
};

struct Jingle {
    struct Content;
    struct Group;
    enum class Action;

    Action                 action;
    std::string            sid;
    std::string            initiator = "";
    std::string            responder = "";
    std::unique_ptr<Group> group;

    std::vector<Content> contents;
};

struct Jingle::Content {
    struct RTPDescription;
    struct IceUdpTransport;
    enum class Senders;

    std::string name;
    Senders     senders;
    bool        is_from_initiator = false;

    std::vector<RTPDescription>  descriptions;
    std::vector<IceUdpTransport> transports;
};

struct Jingle::Content::RTPDescription {
    struct PayloadType;
    struct RTPHeaderExt;
    struct Source;
    struct SSRCGroup;

    std::string media       = "";
    uint32_t    ssrc        = -1;
    bool        support_mux = false;

    std::vector<PayloadType>  payload_types;
    std::vector<Source>       sources;
    std::vector<RTPHeaderExt> rtp_header_exts;
    std::vector<SSRCGroup>    ssrc_groups;
};

struct Jingle::Content::RTPDescription::PayloadType {
    struct RTCPFeedBack;
    using Parameter = Parameter<false>;

    int         id;
    int         clockrate = -1;
    int         channels  = -1;
    std::string name      = "";

    std::vector<RTCPFeedBack> rtcp_fbs;
    std::vector<Parameter>    parameters;
};

struct Jingle::Content::RTPDescription::PayloadType::RTCPFeedBack {
    std::string type;
    std::string subtype = "";
};

struct Jingle::Content::RTPDescription::Source {
    using Parameter = Parameter<true>;

    uint32_t    ssrc;
    std::string name  = "";
    std::string owner = "";

    std::vector<Parameter> parameters;
};

struct Jingle::Content::RTPDescription::RTPHeaderExt {
    int         id;
    std::string uri;
};

struct Jingle::Content::RTPDescription::SSRCGroup {
    enum class Semantics;

    Semantics             semantics;
    std::vector<uint32_t> ssrcs;
};

enum class Jingle::Content::RTPDescription::SSRCGroup::Semantics {
    /// Lip Synchronization, defined in RFC5888.
    Ls,
    /// Flow Identification, defined in RFC5888.
    Fid,
    /// Single Reservation Flow, defined in RFC3524.
    Srf,
    /// Alternative Network Address Types, defined in RFC4091.
    Anat,
    /// Forward Error Correction, defined in RFC4756.
    Fec,
    /// Decoding Dependency, defined in RFC5583.
    Ddp,
};

struct Jingle::Content::IceUdpTransport {
    struct FingerPrint;
    struct Candidate;

    std::string pwd;
    std::string ufrag;
    std::string websocket;
    bool        support_mux = false;

    std::vector<FingerPrint> fingerprints;
    std::vector<Candidate>   candidates;
};

struct Jingle::Content::IceUdpTransport::FingerPrint {
    std::string hash;
    std::string hash_type;
    std::string setup;
    bool        required = false;
};

struct Jingle::Content::IceUdpTransport::Candidate {
    enum class Type;

    uint8_t     component;
    uint8_t     generation;
    uint16_t    port;
    uint32_t    priority;
    Type        type;
    std::string foundation;
    std::string id;
    std::string ip_addr;
};

enum class Jingle::Content::IceUdpTransport::Candidate::Type {
    Host,
    Prflx,
    Relay,
    Srflx,
};

enum class Jingle::Content::Senders {
    Both,
    Initiator,
    Responder,
    None,
};

struct Jingle::Group {
    enum class Semantics;

    Semantics                semantics;
    std::vector<std::string> contents;
};

enum class Jingle::Group::Semantics {
    LipSync,
    Bundle,
};

enum class Jingle::Action {
    /// Accept a content-add action received from another party.
    ContentAccept,
    /// Add one or more new content definitions to the session.
    ContentAdd,
    /// Change the directionality of media sending.
    ContentModify,
    /// Reject a content-add action received from another party.
    ContentReject,
    /// Remove one or more content definitions from the session.
    ContentRemove,
    /// Exchange information about parameters for an application type.
    DescriptionInfo,
    /// Exchange information about security preconditions.
    SecurityInfo,
    /// Definitively accept a session negotiation.
    SessionAccept,
    /// Send session-level information, such as a ping or a ringing message.
    SessionInfo,
    /// Request negotiation of a new Jingle session.
    SessionInitiate,
    /// End an existing session.
    SessionTerminate,
    /// Accept a transport-replace action received from another party.
    TransportAccept,
    /// Exchange transport candidates.
    TransportInfo,
    /// Reject a transport-replace action received from another party.
    TransportReject,
    /// Redefine a transport method or replace it with a different method.
    TransportReplace,

    /// --- Non-standard values used by Jitsi Meet: ---
    /// Add a source to existing content.
    SourceAdd,
    /// Remove a source from existing content.
    SourceRemove,
};

auto parse(const xml::Node& node) -> std::optional<Jingle>;
auto deparse(const Jingle& jingle) -> xml::Node;
} // namespace jingle
