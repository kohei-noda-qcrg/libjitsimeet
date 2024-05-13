subdir('xml')

libjitsimeet_deps = [
  dependency('libwebsockets'),
  dependency('openssl'),
  dependency('nice'),
]

libjitsimeet_src = files(
  'conference.cpp',
  'caps.cpp',
  'sha.cpp',
  'base64.cpp',
  'random.cpp',
  'uri.cpp',
  'websocket.cpp',
  'colibri.cpp',
  'xmpp/negotiator.cpp',
  'xmpp/jid.cpp',
  'xmpp/extdisco.cpp',
  'jingle/parser.cpp',
  'jingle/deparser.cpp',
  'jingle-handler/jingle.cpp',
  'jingle-handler/cert.cpp',
  'jingle-handler/pem.cpp',
  'jingle-handler/hostaddr.cpp',
  'jingle-handler/ice.cpp',
) + tinyxml_src
