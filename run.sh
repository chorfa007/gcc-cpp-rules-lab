#!/usr/bin/env bash
set -e

IMAGE="cpp-reflection-lab:gcc"
CONTAINER="cpp-reflection-lab-gcc"
DOCKERFILE="Dockerfile"

usage() {
  cat <<EOF
Usage: $0 [command]

Commands:
  build     Build (or rebuild) the Docker image
  run       Run an interactive shell inside the container
  exec      Attach to an already-running container
  demo      Build image and immediately run all examples (non-interactive)
  clean     Remove container and image
  push      Tag and push image to Docker Hub  (requires DOCKER_USER env var)
  (no args) Build if needed, then run interactive shell

Examples:
  $0              # interactive shell
  $0 build        # rebuild image
  $0 demo         # run all examples
  $0 clean        # remove image and container

EOF
}

image_exists()      { docker image inspect "$IMAGE"     &>/dev/null; }
container_running() { docker ps -q -f name="$CONTAINER" | grep -q .; }

build() {
  echo ">>> Building $IMAGE …"
  DOCKER_BUILDKIT=0 docker build -t "$IMAGE" -f "$DOCKERFILE" .
  echo ">>> Build complete."
}

run_interactive() {
  image_exists || build
  echo ">>> Starting $IMAGE container (Ctrl-D or 'exit' to leave) …"
  docker run --rm -it \
    -v "$(pwd)/examples:/examples" \
    -e CXX=g++ \
    --name "$CONTAINER" \
    "$IMAGE"
}

case "${1:-}" in
  build)
    build ;;

  run)
    run_interactive ;;

  exec)
    if container_running; then
      docker exec -it "$CONTAINER" /bin/bash
    else
      echo "Container '$CONTAINER' is not running. Use: $0 run"
      exit 1
    fi ;;

  demo)
    image_exists || build
    docker run --rm \
      -v "$(pwd)/examples:/examples" \
      -e CXX=g++ \
      "$IMAGE" bash "./run_all.sh" ;;

  clean)
    docker rm -f "$CONTAINER" 2>/dev/null || true
    docker rmi -f "$IMAGE"    2>/dev/null || true
    echo ">>> Cleaned up $IMAGE." ;;

  push)
    : "${DOCKER_USER:?Set DOCKER_USER=your-dockerhub-username}"
    image_exists || build
    docker tag "$IMAGE" "$DOCKER_USER/$IMAGE"
    docker push "$DOCKER_USER/$IMAGE"
    echo ">>> Pushed $DOCKER_USER/$IMAGE" ;;

  help|-h|--help)
    usage ;;

  "")
    run_interactive ;;

  *)
    echo "Unknown command: $1"
    usage
    exit 1 ;;
esac
