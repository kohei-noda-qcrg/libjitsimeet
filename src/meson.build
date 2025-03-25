subdir('xml')
subdir('json')
subdir('ws')

libjitsimeet_deps = [
  dependency('openssl'),
  dependency('nice'),
  dependency('coop'),
] + ws_deps

libjitsimeet_src = files(
  'async-websocket.cpp',
  'caps.cpp',
  'colibri.cpp',
  'conference.cpp',
  'crypto/base64.cpp',
  'crypto/sha.cpp',
  'jingle-handler/cert.cpp',
  'jingle-handler/hostaddr.cpp',
  'jingle-handler/ice.cpp',
  'jingle-handler/jingle.cpp',
  'jingle-handler/pem.cpp',
  'jingle/jingle.cpp',
  'random.cpp',
  'uri.cpp',
  'xmpp/extdisco.cpp',
  'xmpp/jid.cpp',
  'xmpp/negotiator.cpp',
) + tinyxml_files + tinyjson_files + ws_files + ws_client_files
