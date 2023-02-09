#   Xitter the X/Twitter clone

Xitter is a Twitter clone that is community-centric.
There are two commands:

  1. xitter-http, the HTTP server (Go?)
  2. xitter-client, the client-side app (Python? JavaScript?)

Xitter can use rdx/brix through API bindings or through CLI.
Implementing RDX in JavaScript (Python?) may be needed, but
RDX is a very basic TLV format. There are Go implementations
of RDX (which may need to be updated to catch up with C).

Servers talk to each other by HTTP; to the clients also by
HTTP. (Although, you can also try ssh access and terminal
clients, that is up to you.)

##  Spaces

The idea is to have federated communities or "spaces". Within
a space, everyone sees everything, some can post, some can
invite people. The full access control model is specific to an
implementation or a space and not part of a spec.

A user can be a member of any number of spaces and read them
as a single feed or separately, post to all of them, only one
or to any subset. "Retweeting" is relaying "events" between
"spaces". Nothing prevents us from having single-user spaces.

Technically, each space has a BRIX store containing the entire
history of tweets, either locally originated or relayed. These
BRIX stores are accessed through a very regular REST API.
(Get me a file, post an event, etc).

##  Data formats

"Events" are serialized as RDX E elements:

````
    {@alice-1675916490
        type: tweet
        text: "Hello world!"
        sign: c9a5a0d1d1040985e...
        home: "http://wonderland.com/xiter"
        icon: "http://wonderland.com/xiter/alice3.jpg"
        replies: {
            {@ben-1675916558
                type: tweet
                text: "Sholom"
                home: "http://xiter.il/ben"
                icon: "http://xiter.il/ben/Ben.N.jpg"
            }
        }
    }
````
An object id format is: `userid-timestamp`, using UNIX epoch.
Optionally, an event might have a signature and the public key
should be available at the home space server.
Dependent objects (replies) get nested into the original one.
Note that versions of events are *mergeable*, i.e. we can 
retrieve versions of an event from different spaces and merge.
An event (preferably) references its home space URI, so the 
reader can always fetch the original, merge in the replies to
the original tweet, and so on.
Note that such objects get ordered chronologically in a BRIX 
store.
Server-client communication is RDX over REST, unless you
decide to render everything on the server. Then, the client 
becomes a subroutine of the server.

##  User interface

Something like a Twitter feed, preferably usable by a regular 
person. Although, a console based client might also work, just
add some color.

