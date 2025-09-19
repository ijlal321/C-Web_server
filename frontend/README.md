How to use



.
goto frontend folder.
change env var mode to master_app
pnpm run build
    // this will give master_app frontend to c server
change env to client
pnpm run dev

go outside
make run


now your both apps are running
8080 -> master app + c server
4000 -> client

also you will need to change websocket, see comments in .env of frontend