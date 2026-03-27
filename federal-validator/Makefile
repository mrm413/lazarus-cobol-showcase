# Lazarus C++17 Transpiler — Federal Validation
# Torsova LLC — lazarus-systems.com

DOCKER_IMAGE  := lazarus-cpp17-validator
DOCKER_TAG    := latest
BUILD_CONTEXT := ..

.PHONY: build test test-single report clean docker-build docker-run help

help:  ## Show this help
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | \
		awk 'BEGIN {FS = ":.*?## "}; {printf "  %-18s %s\n", $$1, $$2}'

# ---------------------------------------------------------------------------
# Docker targets (primary usage)
# ---------------------------------------------------------------------------

docker-build:  ## Build the Docker validation image
	docker build -t $(DOCKER_IMAGE):$(DOCKER_TAG) -f Dockerfile $(BUILD_CONTEXT)

docker-run:  ## Run the full validation suite in Docker
	docker run --rm $(DOCKER_IMAGE):$(DOCKER_TAG)

docker-single:  ## Run a single test in Docker (TEST=category/name)
	docker run --rm $(DOCKER_IMAGE):$(DOCKER_TAG) $(TEST)

# ---------------------------------------------------------------------------
# Docker Compose targets
# ---------------------------------------------------------------------------

up:  ## Build and run via docker-compose
	docker-compose up --build

down:  ## Tear down docker-compose
	docker-compose down -v

# ---------------------------------------------------------------------------
# Local targets (for use inside the container or on a Linux host)
# ---------------------------------------------------------------------------

test:  ## Run the full test suite locally
	./test_runner.sh

test-single:  ## Run a single test (TEST=category/name)
	./test_runner.sh $(TEST)

report:  ## Print the last validation summary
	@if [ -f results/summary.log ]; then \
		cat results/summary.log; \
	else \
		echo "No results found. Run 'make test' first."; \
	fi

failures:  ## Print failure details
	@if [ -f results/failures.log ]; then \
		cat results/failures.log; \
	else \
		echo "No failures recorded."; \
	fi

safety:  ## Print safety audit results
	@if [ -f results/safety_audit.log ]; then \
		cat results/safety_audit.log; \
	else \
		echo "No safety findings."; \
	fi

clean:  ## Remove build artifacts and results
	rm -rf results/
