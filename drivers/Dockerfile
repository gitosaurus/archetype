# Use the official lightweight Python image.
# https://hub.docker.com/_/python
FROM python:3.10-slim

# Prepare to build the main Archetype program.

RUN apt update && \
    apt install --yes \
        cmake g++

# First build the main Archetype executable.
WORKDIR /c/source
COPY archetype/ /c/source/

# Run the CMake configuration step, setting the options to create
# a statically linked C++ program
RUN cmake -S/c/source -B/c/binary \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_EXE_LINKER_FLAGS=-static

RUN cmake --build /c/binary
RUN cmake --install /c/binary --prefix /usr/local

# Copy local code to the container image.
ENV APP_HOME /app
WORKDIR $APP_HOME
COPY cloud_run/ ./

# Install production dependencies.
RUN pip install --no-cache-dir -r requirements.txt

# Allow statements and log messages to immediately appear in the Knative logs
ENV PYTHONUNBUFFERED True

# Run the web service on container startup. Here we use the gunicorn
# webserver, with one worker process and 8 threads.
# For environments with multiple CPU cores, increase the number of workers
# to be equal to the cores available.
# Timeout is set to 0 to disable the timeouts of the workers to allow Cloud Run to handle instance scaling.
CMD exec gunicorn --bind :$PORT --workers 1 --threads 8 --timeout 0 main:app
