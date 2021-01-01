Import("env")
env.Replace( MKSPIFFSTOOL='python3" "' + env.get("PROJECT_DIR") + '/mklittlefs.py' )