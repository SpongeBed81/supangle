if [ "$EUID" -ne 0 ]
then 
  echo "Please run with superuser privileges."
  exit
fi

if ! command -v "nodemon" /dev/null
then
  npm i -g nodemon --silent
fi

if ! command -v "ccache" /dev/null
then
  source /etc/os-release
  if [ "$ID" = "ubuntu" ]
  then
    apt-get update
    apt-get install -y ccache
  fi
  if [ "$ID" = "fedora" ]
  then
    dnf install -y ccache
  fi
fi

nodemon -e cc,h,js,cpp,hpp --exec "make && ./bin/supangle index.js"