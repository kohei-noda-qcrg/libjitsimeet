#pragma once

namespace xmpp {
enum class UserStats {
    /// 100: Inform user that any occupant is allowed to see the user's full JID
    NonAnonymousRoom = 100,

    /// 101: Inform user that his or her affiliation changed while not in the room
    AffiliationChange = 101,

    /// 102: Inform occupants that room now shows unavailable members
    ConfigShowsUnavailableMembers = 102,

    /// 103: Inform occupants that room now does not show unavailable members
    ConfigHidesUnavailableMembers = 103,

    /// 104: Inform occupants that a non-privacy-related room configuration change has occurred
    ConfigNonPrivacyRelated = 104,

    /// 110: Inform user that presence refers to itself
    SelfPresence = 110,

    /// 170: Inform occupants that room logging is now enabled
    ConfigRoomLoggingEnabled = 170,

    /// 171: Inform occupants that room logging is now disabled
    ConfigRoomLoggingDisabled = 171,

    /// 172: Inform occupants that the room is now non-anonymous
    ConfigRoomNonAnonymous = 172,

    /// 173: Inform occupants that the room is now semi-anonymous
    ConfigRoomSemiAnonymous = 173,

    /// 201: Inform user that a new room has been created
    RoomHasBeenCreated = 201,

    /// 210: Inform user that service has assigned or modified occupant's roomnick
    AssignedNick = 210,

    /// 301: Inform user that they have been banned from the room
    Banned = 301,

    /// 303: Inform all occupants of new room nickname
    NewNick = 303,

    /// 307: Inform user that they have been kicked from the room
    Kicked = 307,

    /// 321: Inform user that they are being removed from the room
    /// because of an affiliation change
    RemovalFromRoom = 321,

    /// 322: Inform user that they are being removed from the room
    /// because the room has been changed to members-only and the
    /// user is not a member
    ConfigMembersOnly = 322,

    /// 332: Inform user that they are being removed from the room
    /// because the MUC service is being shut down
    ServiceShutdown = 332,

    /// 333: Inform user that they are being removed from the room for technical reasons
    ServiceErrorKick = 333,
};
}
