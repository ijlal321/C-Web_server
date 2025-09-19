How to use



.
goto frontend folder.
change env var mode to client
pnpm run build
    // this will give client frontend to c server
change env to master_app
pnpm run dev

go outside
make run


now your both apps are running
8080 -> client + c server
4000 -> master app

also you will need to change websocket, see comments in .env of frontend