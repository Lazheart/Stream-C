FROM gcc:latest

# Install required dependencies
RUN apt-get update && apt-get install -y \
    libcurl4-openssl-dev \
    make \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy the project files
COPY . .

# Build the data fetcher and fetch data
RUN make data/data.csv

# Build the CLI application
RUN make cli

# Run the CLI application by default
CMD ["./cli"]
