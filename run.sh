#!/usr/bin/env bash
set -e

IMAGE="gcc-cpp-rules-lab:latest"
CONTAINER="cpp-rules-lab"

# ── helpers ───────────────────────────────────────────────────────────────────
usage() {
  cat <<EOF
Usage: $0 [command]

Commands:
  build       Build (or rebuild) the Docker image
  run         Run an interactive shell inside the container
  exec        Attach to an already-running container
  demo        Build image and immediately run all examples (non-interactive)
  clean       Remove container and image
  push        Tag and push image to Docker Hub  (requires DOCKER_USER env var)
  (no args)   Build if needed, then run interactive shell

EOF
}

image_exists()     { docker image inspect "$IMAGE"     &>/dev/null; }
container_running(){ docker ps -q -f name="$CONTAINER" | grep -q .; }

build() {
  echo ">>> Building $IMAGE …"
  docker build -t "$IMAGE" .
  echo ">>> Build complete."
}

run_interactive() {
  image_exists || build
  echo ">>> Starting container (Ctrl-D or 'exit' to leave) …"
  docker run --rm -it \
    -v "$(pwd)/examples:/examples" \
    --name "$CONTAINER" \
    "$IMAGE"
}

# ── main ──────────────────────────────────────────────────────────────────────
case "${1:-}" in
  build)
    build ;;

  run)
    run_interactive ;;

  exec)
    if container_running; then
      docker exec -it "$CONTAINER" /bin/bash
    else
      echo "Container is not running. Use: $0 run"
      exit 1
    fi ;;

  demo)
    image_exists || build
    docker run --rm -it "$IMAGE" bash -c "./run_all.sh" ;;

  clean)
    docker rm -f "$CONTAINER" 2>/dev/null || true
    docker rmi -f "$IMAGE"    2>/dev/null || true
    echo ">>> Cleaned up." ;;

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
