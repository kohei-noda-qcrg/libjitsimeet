subdir('xml')

libjitsimeet_deps = [
  dependency('libwebsockets'),
  dependency('openssl'),
]

libjitsimeet_src = files(
  'conference.cpp',
  'caps.cpp',
  'sha.cpp',
  'base64.cpp',
  'websocket.cpp',
  'xmpp/connection.cpp',
  'xmpp/jid.cpp',
  'xmpp/extdisco.cpp',
  'jingle/parser.cpp',
  'jingle/deparser.cpp',
) + tinyxml_src
