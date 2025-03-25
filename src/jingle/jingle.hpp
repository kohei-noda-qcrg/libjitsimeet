#pragma once
#include <vector>

#include "../xml/xml.hpp"
#include "serde/serde.hpp"

namespace jingle {
enum class Action {
    ContentAccept,    // Accept a content-add action received from another party.
    ContentAdd,       // Add one or more new content definitions to the session.
    ContentModify,    // Change the directionality of media sending.
    ContentReject,    // Reject a content-add action received from another party.
    ContentRemove,    // Remove one or more content definitions from the session.
    DescriptionInfo,  // Exchange information about parameters for an application type.
    SecurityInfo,     // Exchange information about security preconditions.
    SessionAccept,    // Definitively accept a session negotiation.
    SessionInfo,      // Send session-level information, such as a ping or a ringing message.
    SessionInitiate,  // Request negotiation of a new Jingle session.
    SessionTerminate, // End an existing session.
    TransportAccept,  // Accept a transport-replace action received from another party.
    TransportInfo,    // Exchange transport candidates.
    TransportReject,  // Reject a transport-replace action received from another party.
    TransportReplace, // Redefine a transport method or replace it with a different method.
    SourceAdd,        // Non-standard, Add a source to existing content.
    SourceRemove,     // Non-standard, Remove a source from existing content.
};

enum class Senders {
    Both,
    Initiator,
    Responder,
    None,
};

enum class SSRCSemantics {
    Ls,   /// Lip Synchronization, defined in RFC5888.
    Fid,  /// Flow Identification, defined in RFC5888.
    Srf,  /// Single Reservation Flow, defined in RFC3524.
    Anat, /// Alternative Network Address Types, defined in RFC4091.
    Fec,  /// Forward Error Correction, defined in RFC4756.
    Ddp,  /// Decoding Dependency, defined in RFC5583.
};

enum class CandidateType {
    Host,
    Prflx,
    Relay,
    Srflx,
};

enum class GroupSemantics {
    LipSync,
    Bundle,
};

template <class T>
struct XMLNameSpace {};

struct Parameter {
    SerdeFieldsBegin;
    std::string SerdeField(name);
    std::string SerdeField(value);
    SerdeFieldsEnd;
};

struct RTCPFeedBack {
    constexpr static auto ns = "urn:xmpp:jingle:apps:rtp:rtcp-fb:0";

    SerdeFieldsBegin;
    [[no_unique_address]]
    XMLNameSpace<RTCPFeedBack> SerdeField(xmlns);
    std::string                SerdeField(type);
    std::optional<std::string> SerdeField(subtype);
    SerdeFieldsEnd;
};

struct PayloadType {
    SerdeFieldsBegin;
    int                        SerdeField(id);
    std::optional<int>         SerdeField(clockrate);
    std::optional<int>         SerdeField(channels);
    std::optional<std::string> SerdeField(name);

    std::vector<RTCPFeedBack> SerdeNamedField(rtcp_fb, "rtcp-fb");
    std::vector<Parameter>    SerdeField(parameter);
    SerdeFieldsEnd;
};

struct Owner {
    constexpr static auto ns = "http://jitsi.org/jitmeet";

    SerdeFieldsBegin;
    [[no_unique_address]]
    XMLNameSpace<Owner> SerdeField(xmlns);
    std::string         SerdeField(owner);
    SerdeFieldsEnd;
};

struct Source {
    constexpr static auto ns = "urn:xmpp:jingle:apps:rtp:ssma:0";

    SerdeFieldsBegin;
    [[no_unique_address]]
    XMLNameSpace<Source>       SerdeField(xmlns);
    uint32_t                   SerdeField(ssrc);
    std::optional<std::string> SerdeField(name);
    std::optional<std::string> SerdeNamedField(video_type, "videoType");

    std::vector<Parameter> SerdeField(parameter);
    std::vector<Owner>     SerdeNamedField(ssrc_info, "ssrc-info");
    SerdeFieldsEnd;
};

struct RTPHeaderExt {
    constexpr static auto ns = "urn:xmpp:jingle:apps:rtp:rtp-hdrext:0";

    SerdeFieldsBegin;
    [[no_unique_address]]
    XMLNameSpace<RTPHeaderExt> SerdeField(xmlns);
    int                        SerdeField(id);
    std::string                SerdeField(uri);
    SerdeFieldsEnd;
};

struct SSRC {
    SerdeFieldsBegin;
    uint32_t SerdeField(ssrc);
    SerdeFieldsEnd;
};

struct SSRCGroup {
    constexpr static auto ns = "urn:xmpp:jingle:apps:rtp:ssma:0";

    SerdeFieldsBegin;
    [[no_unique_address]]
    XMLNameSpace<SSRCGroup> SerdeField(xmlns);
    SSRCSemantics           SerdeField(semantics);

    std::vector<SSRC> SerdeField(source);
    SerdeFieldsEnd;
};

struct RTPDescription {
    constexpr static auto ns = "urn:xmpp:jingle:apps:rtp:1";

    SerdeFieldsBegin;
    [[no_unique_address]]
    XMLNameSpace<RTPDescription> SerdeField(xmlns);
    std::optional<std::string>   SerdeField(media);
    std::optional<uint32_t>      SerdeField(ssrc);

    std::vector<PayloadType>  SerdeNamedField(payload_type, "payload-type");
    std::vector<Source>       SerdeField(source);
    std::vector<RTPHeaderExt> SerdeNamedField(rtp_header_ext, "rtp-hdrext");
    std::vector<SSRCGroup>    SerdeNamedField(ssrc_group, "ssrc-group");
    // TODO: handle "rtcp-mux";
    SerdeFieldsEnd;
};

struct ColibriWebSocket {
    constexpr static auto ns = "http://jitsi.org/protocol/colibri";

    SerdeFieldsBegin;
    [[no_unique_address]]
    XMLNameSpace<ColibriWebSocket> SerdeField(xmlns);
    std::string                    SerdeField(url);
    SerdeFieldsEnd;
};

struct FingerPrint {
    constexpr static auto ns = "urn:xmpp:jingle:apps:dtls:0";

    SerdeFieldsBegin;
    [[no_unique_address]]
    XMLNameSpace<FingerPrint>  SerdeField(xmlns);
    std::string                SerdeField(hash);
    std::string                SerdeField(setup);
    std::optional<std::string> SerdeField(required);
    SerdeFieldsEnd;

    std::string data; // hash body
};

struct Candidate {
    SerdeFieldsBegin;
    uint8_t       SerdeField(component);
    uint8_t       SerdeField(generation);
    uint16_t      SerdeField(port);
    uint32_t      SerdeField(priority);
    CandidateType SerdeField(type);
    std::string   SerdeField(foundation);
    std::string   SerdeField(id);
    std::string   SerdeField(ip);
    std::string   SerdeField(protocol);
    SerdeFieldsEnd;
};

struct IceUdpTransport {
    constexpr static auto ns = "urn:xmpp:jingle:transports:ice-udp:1";

    SerdeFieldsBegin;
    [[no_unique_address]]
    XMLNameSpace<IceUdpTransport> SerdeField(xmlns);
    std::string                   SerdeField(pwd);
    std::string                   SerdeField(ufrag);

    std::vector<ColibriWebSocket> SerdeNamedField(websocket, "web-socket");
    std::vector<FingerPrint>      SerdeField(fingerprint);
    std::vector<Candidate>        SerdeField(candidate);
    // TODO: handle rtcp-mux

    SerdeFieldsEnd;
};

struct Content {
    SerdeFieldsBegin;
    std::string                  SerdeField(name);
    std::optional<Senders>       SerdeField(senders);
    std::optional<std::string>   SerdeField(creator);
    std::vector<RTPDescription>  SerdeField(description);
    std::vector<IceUdpTransport> SerdeField(transport);
    SerdeFieldsEnd;
};

struct GroupContent {
    SerdeFieldsBegin;
    std::string SerdeField(name);
    SerdeFieldsEnd;
};

struct Group {
    constexpr static auto ns = "urn:xmpp:jingle:apps:grouping:0";

    SerdeFieldsBegin;
    [[no_unique_address]]
    XMLNameSpace<Group> SerdeField(xmlns);
    GroupSemantics      SerdeField(semantics);

    std::vector<GroupContent> SerdeField(content);
    SerdeFieldsEnd;
};

struct Jingle {
    constexpr static auto ns = "urn:xmpp:jingle:1";

    SerdeFieldsBegin;
    [[no_unique_address]]
    XMLNameSpace<Jingle>       SerdeField(xmlns);
    Action                     SerdeField(action);
    std::string                SerdeField(sid);
    std::optional<std::string> SerdeField(initiator);
    std::optional<std::string> SerdeField(responder);
    std::vector<Content>       SerdeField(content);
    std::vector<Group>         SerdeField(group);
    SerdeFieldsEnd;
};

auto parse(const xml::Node& node) -> std::optional<Jingle>;
auto deparse(const Jingle& jingle) -> std::optional<xml::Node>;
} // namespace jingle
