/**
 * ============================================================================
 * EDGE HUB - Main Application Script
 * ============================================================================
 * 
 * This script provides the core functionality for the Edge HUB web application,
 * including:
 * 
 * 1. Configuration Management
 *    - Loads configuration from server API endpoint
 *    - Merges with default configuration values
 *    - Provides fallback defaults if server config unavailable
 * 
 * 2. Sensor Dashboard
 *    - Real-time sensor data visualization using Plotly.js
 *    - Four multi-line plots: Temperature, Voltage, Current, Power
 *    - Metric cards with trend indicators (up/down/neutral)
 *    - Automatic data updates at configurable intervals
 * 
 * 3. Live Camera Streaming
 *    - Support for CPNX (Pi Camera) and CLNX (USB Camera)
 *    - Dual camera mode with synchronized streaming
 *    - Camera control (start/stop) via API
 *    - Real-time status checking and error handling
 *    - FPS display updates based on camera mode (5 FPS dual, 30 FPS single)
 * 
 * 4. Image Operations
 *    - Capture images from GARD system (supports RAW/bin and standard image formats)
 *    - Save images to client (Downloads/edgeHUB) or server
 *    - File System Access API integration for client-side saves
 *    - RAW/bin format support with binary-to-PNG conversion for display
 *    - Image format detection and conversion (PNG, JPEG, BMP, RAW/bin)
 *    - Anomaly status tagging (normal/anomaly) for image classification
 *    - Configurable filename generation with timestamps and classification tags
 * 
 * 5. UI Navigation & Layout
 *    - Tab-based navigation (Sensor Dashboard, Live Streaming, Image Ops)
 *    - Carousel for home view
 *    - Modal dialogs for thresholds, ISP settings, circuit viewer
 *    - Responsive layout management
 * 
 * 6. Data Management
 *    - LocalStorage for saving user preferences (save paths)
 *    - Sliding window data management for plots (configurable window size)
 *    - Trend tracking for metric values
 * 
 * Dependencies:
 *    - Plotly.js (for graph visualization)
 *    - Font Awesome (for icons)
 *    - Modern browser with ES6+ support
 * 
 * ============================================================================
 */

document.addEventListener('DOMContentLoaded', async () => {
    // ========================================================================
    // SECTION 1: CONFIGURATION LOADING
    // ========================================================================
    
    let CONFIG = {};
    try {
        const configEndpoint = '/api/config';
        const configResponse = await fetch(configEndpoint);
        if (configResponse.ok) {
            CONFIG = await configResponse.json();
        } else {
            console.warn('Failed to load config, using defaults');
        }
    } catch (error) {
        console.warn('Error loading config:', error, 'using defaults');
    }
    
    // Default configuration values
    const DEFAULT_CONFIG = {
        cameras: { 
            usb: { width: 1920, height: 1080, display_name: 'MIPI Passthrough CLNX USB Camera Feed' }, 
            pi: { width: 3280, height: 2464, display_name: 'MIPI Passthrough CPNX Pi Camera Feed' } 
        },
        paths: { 
            default_server_save_path: '/home/lattice/Downloads/edgeHUB/', 
            default_client_save_path: 'Downloads', 
            save_folder_name: 'edgeHUB' 
        },
        image_ops: { 
            filename_prefix: 'edgehub_imageops', 
            timestamp_format: 'YYYY-MM-DDTHH-mm-ss', 
            image_format: 'png' 
        },
        api_endpoints: { 
            capture_image: '/api/capture_image_from_gard', 
            save_image: '/api/save_image', 
            video_feed_cpnx: '/video_feed/cpnx', 
            video_feed_clnx: '/video_feed/clnx' 
        },
        local_storage_keys: { 
            client_save_path: 'edgeHUB_client_save_path', 
            server_save_path: 'edgeHUB_server_save_path' 
        },
        messages: { 
            info_text: 'Image will be downloaded in <strong>edgeHUB</strong> folder at the given location. By default it is set to Downloads folder.<br>Filename will be auto-generated as: <code style="color: var(--lattice-yellow); font-size: 0.85rem;">edgehub_imageops_&lt;classification&gt;_&lt;timestamp&gt;.png</code>' 
        },
        data_window_size: 40
    };
    
    // Merge server config with defaults
    CONFIG = { ...DEFAULT_CONFIG, ...CONFIG };
    
    // ========================================================================
    // SECTION 2: DOM ELEMENT SELECTION
    // ========================================================================
    
    const tabBtns = document.querySelectorAll('.tab-btn');
    const tabPanels = document.querySelectorAll('.tab-panel');
    const dashboardContainer = document.querySelector('.dashboard-container');
    const carouselContainer = document.getElementById('carousel-container');
    const exitBtn = document.getElementById('exit-btn');
    
    // ========================================================================
    // SECTION 3: STATE & DATA MANAGEMENT
    // ========================================================================
    
    const dataWindowSize = CONFIG.data_window_size || 40;
    
    // Larger window size for voltage plot to show more fluctuations
    const voltageWindowSize = CONFIG.voltage_window_size || 100;
    
    // Global data store for sensor plots
    const multiPlotData = {
        temp: { x: [], traces: { temp1: [], temp2: [], temp3: [] } },
        voltage: { x: [], traces: { voltage1: [], voltage2: [] } },
        current: { x: [], traces: { current1: [], current2: [] } },
        power: { x: [], traces: { power1: [], power2: [] } }
    };
    
    // Track previous values for trend indicators
    let previousValues = {};
    
    // Threshold storage per metric
    let thresholds = {};
    
    // Track first reading per metric for default threshold
    let firstReadings = {};
    
    // Data update interval control
    let dataUpdateInterval = null;
    
    // Image operations state
    let currentImageData = null;
    let selectedDirectoryHandle = null;
    let currentBinData = null;
    
    // ========================================================================
    // SECTION 4: SENSOR DASHBOARD & PLOTTING FUNCTIONS
    // ========================================================================
    
    /**
     * Initializes all four sensor graphs (Temperature, Voltage, Current, Power).
     * 
     * This function is called when switching to the Sensor Dashboard tab to set up
     * the Plotly graphs with their initial configuration. It retrieves trace names
     * from config and creates multi-line plots for each sensor type using distinct
     * colors from Plotly's color scale.
     */
    function initAllSensorGraphs() {
        const colors = Plotly.d3.scale.category10().range();
        const traceNames = CONFIG.plot_trace_names || {};
        
        createMultiLinePlot('plot-temp', traceNames.temp || ['North Sensor Temperature', 'South Sensor Temperature', 'CM5 Temperature'], multiPlotData.temp, [colors[0], colors[1], colors[2]]);
        createMultiLinePlot('plot-voltage', traceNames.voltage || ['CM5 Voltage', 'SOM Voltage'], multiPlotData.voltage, [colors[4], colors[5]]);
        createMultiLinePlot('plot-current', traceNames.current || ['CM5 Current', 'SOM Current'], multiPlotData.current, [colors[6], colors[7]]);
        createMultiLinePlot('plot-power', traceNames.power || ['CM5 Power', 'SOM Power'], multiPlotData.power, [colors[8], colors[9]]);
    }
    
    /**
     * Creates or updates a single multi-line Plotly graph.
     * 
     * This function configures and renders a Plotly multi-line plot with customizable
     * traces, colors, and layout. It determines axis labels and tick settings based
     * on the plot type (temperature, voltage, current, or power). The function handles
     * dynamic X-axis label rotation based on data point count and creates trace objects
     * for each metric with distinct line styles and marker symbols.
     * 
     * @param {string} plotId - DOM element ID for the plot container
     * @param {string[]} traceNames - Array of trace names to display
     * @param {Object} data - Data store object with x array and traces object
     * @param {string[]} lineColors - Array of color values for each trace
     */
    function createMultiLinePlot(plotId, traceNames, data, lineColors) {
        let yLabel = '';
        let yDTick = undefined;
        
        if (plotId === 'plot-temp') {
            yLabel = 'Temperature (°C)';
            yDTick = 0.5;
        } else if (plotId === 'plot-voltage') {
            yLabel = 'Voltage (V)';
            yDTick = 0.5;
        } else if (plotId === 'plot-current') {
            yLabel = 'Current (A)';
            yDTick = 0.05;
        } else if (plotId === 'plot-power') {
            yLabel = 'Power (W)';
        }
        
        const bins = data.x ? data.x.length : 0;
        const tickAngle = bins > 5 ? 45 : 0;
        const markerSymbols = ['circle', 'square', 'diamond', 'triangle-up'];
        
        const traces = traceNames.map((name, index) => {
            const metricName = document.querySelector(`[data-title="${name}"]`)?.dataset.metric || name.toLowerCase().replace(/\s/g, '');
            const markerSymbol = markerSymbols[index % markerSymbols.length];
            
            return {
                x: data.x,
                y: data.traces[metricName] || [],
                mode: 'lines+markers',
                name: name,
                line: { 
                    color: lineColors[index], 
                    width: 2.5,
                },
                marker: { 
                    color: lineColors[index], 
                    size: 8,
                    symbol: markerSymbol,
                    line: { width: 2, color: 'white' }
                },
                legendgroup: name,
                showlegend: true,
                hoverlabel: {
                    namelength: -1,
                    bgcolor: '#111',
                    font: { color: '#fff', size: 14 },
                    bordercolor: '#ffc107'
                }
            };
        });
        
        const layout = {
            paper_bgcolor: 'rgba(0,0,0,0)',
            plot_bgcolor: 'rgba(0,0,0,0)',
            font: { color: '#b8b8b8' },
            showlegend: true,
            legend: {
                orientation: 'h',
                y: -0.25,
                yanchor: 'top',
                x: 0.5,
                xanchor: 'center',
                font: { size: 16 },
                itemsizing: 'constant',
            },
            margin: { t: 5, l: 50, r: 20, b: 80 },
            xaxis: {
                gridcolor: 'rgba(255, 255, 255, 0.1)',
                tickformat: '%H:%M:%S',
                automargin: true,
                zeroline: false,
                title: { text: 'Time', font: { size: 16, color: '#ffc107' } },
                nticks: 10,
                tickangle: tickAngle,
            },
            yaxis: {
                gridcolor: 'rgba(255, 255, 255, 0.1)',
                automargin: true,
                zeroline: false,
                title: { text: yLabel, font: { size: 16, color: '#ffc107' } },
                tickmode: yDTick ? 'linear' : undefined,
                dtick: yDTick,
                nticks: plotId === 'plot-voltage' ? 21 : undefined,
            },
            hoverlabel: {
                bgcolor: '#111',
                font: { color: '#fff', size: 14 },
                bordercolor: '#ffc107',
                namelength: -1
            }
        };
        
        if (plotId === 'plot-voltage') {
            layout.yaxis.range = [0, 10];
        }
        
        const plotName = plotId.replace('plot-', '');
        const timestamp = new Date().toISOString().replace(/[:.]/g, '-').slice(0, -5);
        
        Plotly.newPlot(plotId, traces, layout, { 
            responsive: true, 
            displayModeBar: true,
            modeBarButtonsToAdd: [],
            modeBarButtonsToRemove: [],
            displaylogo: false,
            toImageButtonOptions: {
                format: 'png',
                filename: `edgehub_sensorgraph_${plotName}_${timestamp}`,
                height: 600,
                width: 1200,
                scale: 1
            }
        });
    }
    
    /**
     * Fetches live sensor data from the server and updates the dashboard.
     * 
     * This function performs a periodic update of all sensor data displayed on the dashboard.
     * It fetches the latest sensor readings from the server API, updates metric card values
     * with formatted numbers, calculates and displays trend indicators (up/down/neutral arrows)
     * based on value changes, updates the sliding window data stores for all plot graphs,
     * and refreshes the Plotly graphs if the sensor dashboard is currently visible. The function
     * also manages threshold comparisons for visual feedback (color changes) and maintains
     * sliding windows of data points for each sensor metric.
     */
    const updateDashboard = async () => {
        try {
            const response = await fetch(CONFIG.api_endpoints.get_live_data || '/get_live_data');
            if (!response.ok) return;
            
            const data = await response.json();
            const timestamp = new Date();
            
            document.querySelectorAll('.metric-card[data-metric]').forEach(card => {
                const metric = card.dataset.metric;
                const newValue = data[metric];
                
                if (newValue !== undefined) {
                    const valueElement = document.getElementById(`${metric}-value`);
                    if (valueElement) {
                        valueElement.textContent = `${newValue.toFixed(2)}${card.dataset.unit}`;
                    }
                    
                    const trendElement = document.getElementById(`${metric}-trend`);
                    const oldValue = previousValues[metric];
                    
                    if (trendElement && oldValue !== undefined) {
                        let iconClass = 'fas fa-minus';
                        if (newValue > oldValue) {
                            iconClass = 'fas fa-arrow-up';
                            trendElement.style.color = '#dc3545'; // Red for increase
                        } else if (newValue < oldValue) {
                            iconClass = 'fas fa-arrow-down';
                            trendElement.style.color = '#28a745';
                        } else {
                            trendElement.style.color = '#ffc107';
                        }
                        trendElement.innerHTML = `<i class="${iconClass}"></i>`;
                    }
                    
                    previousValues[metric] = newValue;
                }
            });
            
            const allMetrics = CONFIG.plot_metric_keys || {
                temp: ['temp1', 'temp2', 'temp3'],
                voltage: ['voltage1', 'voltage2'],
                current: ['current1', 'current2'],
                power: ['power1', 'power2']
            };
            
            Object.keys(allMetrics).forEach(plotKey => {
                const plotStore = multiPlotData[plotKey];
                plotStore.x.push(timestamp);
                
                const windowSize = (plotKey === 'voltage') ? voltageWindowSize : dataWindowSize;
                
                allMetrics[plotKey].forEach(metricKey => {
                    if (!plotStore.traces[metricKey]) {
                        plotStore.traces[metricKey] = [];
                    }
                    const currentValue = data[metricKey];
                    plotStore.traces[metricKey].push(currentValue);
                    
                    if (thresholds[metricKey] === undefined && currentValue !== undefined) {
                        thresholds[metricKey] = currentValue;
                        firstReadings[metricKey] = currentValue;
                    }
                    
                    if (plotStore.traces[metricKey].length > windowSize) {
                        plotStore.traces[metricKey].shift();
                    }
                });
                
                if (plotStore.x.length > windowSize) {
                    plotStore.x.shift();
                }
            });
            
            if (dashboardContainer.classList.contains('sensor-view-active')) {
                const updatePlot = (plotId, dataStore, traceKeys, traceNames) => {
                    const updateData = { x: [], y: [] };
                    const lineColors = [];
                    const markerColors = [];
                    
                    for (let i = 0; i < traceKeys.length; i++) {
                        const metricKey = traceKeys[i];
                        const traceData = dataStore.traces[metricKey] || [];
                        updateData.x.push(dataStore.x);
                        updateData.y.push(traceData);
                        
                        if (thresholds[metricKey] !== undefined && traceData.length > 0) {
                            const currentValue = traceData[traceData.length - 1];
                            const threshold = thresholds[metricKey];
                            
                            let color;
                            if (currentValue > threshold) {
                                color = '#dc3545';
                            } 
                            else if (currentValue < threshold) {
                                color = '#28a745';
                            } 
                            else {
                                color = '#808080';
                            }
                            
                            lineColors.push(color);
                            markerColors.push(color);
                        } else {
                            lineColors.push(null);
                            markerColors.push(null);
                        }
                    }
                    
                    // Update plot data
                    Plotly.update(plotId, updateData);
                    
                    // Update colors if thresholds are set - update each trace individually
                    lineColors.forEach((color, index) => {
                        if (color !== null) {
                            Plotly.restyle(plotId, {
                                'line.color': color,
                                'marker.color': color
                            }, [index]);
                        }
                    });
                };
                
                const traceNames = CONFIG.plot_trace_names || {};
                updatePlot('plot-temp', multiPlotData.temp, allMetrics.temp, traceNames.temp || ['North Sensor Temperature', 'South Sensor Temperature', 'CM5 Temperature']);
                updatePlot('plot-voltage', multiPlotData.voltage, allMetrics.voltage, traceNames.voltage || ['CM5 Voltage', 'SOM Voltage']);
                updatePlot('plot-current', multiPlotData.current, allMetrics.current, traceNames.current || ['CM5 Current', 'SOM Current']);
                updatePlot('plot-power', multiPlotData.power, allMetrics.power, traceNames.power || ['CM5 Power', 'SOM Power']);
            }
        } catch (error) {
            const errorMsg = CONFIG.errors?.dashboard_update_failed || "Dashboard update failed:";
            console.error(errorMsg, error);
        }
    };
    
    /**
     * Starts the periodic data update interval.
     * Only updates when sensor dashboard or live streaming is active.
     */
    function startDataUpdates() {
        if (dataUpdateInterval) return; // Already running
        
        updateDashboard().then(() => {
            const updateInterval = CONFIG.delays?.data_update_interval || 1000;
            dataUpdateInterval = setInterval(() => {
                if (dashboardContainer.classList.contains('sensor-view-active') || 
                    dashboardContainer.classList.contains('live-streaming-active')) {
                    updateDashboard();
                }
            }, updateInterval);
        });
    }
    
    /**
     * Stops the periodic data update interval.
     * 
     * This function clears the interval timer that was started by startDataUpdates(),
     * effectively stopping automatic sensor data updates. It checks if an interval
     * is active before attempting to clear it, and resets the dataUpdateInterval
     * variable to null after clearing.
     */
    function stopDataUpdates() {
        if (dataUpdateInterval) {
            clearInterval(dataUpdateInterval);
            dataUpdateInterval = null;
        }
    }
    
    // ========================================================================
    // SECTION 5: CAMERA CONTROL FUNCTIONS
    // ========================================================================
    
    /**
     * Shows the video stream and hides the stopped message.
     * 
     * This function updates the UI to display an active camera stream by making
     * the video element visible, hiding the "stopped" message element, and showing
     * the camera info overlay (FPS, resolution). It safely handles null elements
     * by checking for their existence before manipulating their display properties.
     * 
     * @param {HTMLElement} videoElement - Video element to show
     * @param {HTMLElement} msgElement - Message element to hide
     * @param {HTMLElement} infoElement - Info element to show
     */
    function showStream(videoElement, msgElement, infoElement) {
        if (videoElement) videoElement.style.display = 'block';
        if (msgElement) msgElement.style.display = 'none';
        if (infoElement) infoElement.style.display = 'block';
    }
    
    /**
     * Hides the video stream and shows the stopped message.
     * 
     * This function updates the UI to indicate a stopped camera by hiding the
     * video element, displaying the "stopped" message, and hiding the camera
     * info overlay. It safely handles null elements by checking for their
     * existence before manipulating their display properties.
     * 
     * @param {HTMLElement} videoElement - Video element to hide
     * @param {HTMLElement} msgElement - Message element to show
     * @param {HTMLElement} infoElement - Info element to hide
     */
    function showStoppedMsg(videoElement, msgElement, infoElement) {
        if (videoElement) videoElement.style.display = 'none';
        if (msgElement) msgElement.style.display = 'block';
        if (infoElement) infoElement.style.display = 'none';
    }
    
    /**
     * Updates FPS display elements based on camera mode.
     * 
     * This function updates all FPS display elements across the UI to reflect the
     * current camera mode. In dual camera mode, the FPS is set to 5 to reduce
     * bandwidth and processing load. In single camera mode, the FPS is set to 30
     * for smoother video. The function iterates through all known FPS element IDs
     * and updates their text content.
     * 
     * @param {boolean} isDualMode - Whether dual camera mode is active
     */
    function updateFPSDisplays(isDualMode) {
        const fps = isDualMode ? 5 : 30;
        const fpsElements = ['cpnx-fps', 'clnx-fps', 'dual-cpnx-fps', 'dual-clnx-fps'];
        
        fpsElements.forEach(id => {
            const element = document.getElementById(id);
            if (element) {
                element.textContent = fps;
            }
        });
    }
    
    /**
     * Displays a camera error message.
     * 
     * This function handles error display for camera operations by hiding the
     * video element, showing the error message in the stopped message element,
     * and hiding the camera info overlay. It safely handles null elements and
     * updates the message text content with the provided error message.
     * 
     * @param {string} cameraType - Type of camera ('cpnx', 'clnx', or 'dual')
     * @param {string} message - Error message to display
     */
    function showCameraError(cameraType, message) {
        const videoElement = document.getElementById(`${cameraType}-video`);
        const msgElement = document.querySelector(`#${cameraType}-camera-panel .camera-stopped-msg`);
        const infoElement = document.getElementById(`${cameraType}-info`);
        
        if (videoElement) videoElement.style.display = 'none';
        if (msgElement) {
            msgElement.textContent = message;
            msgElement.style.display = 'block';
        }
        if (infoElement) infoElement.style.display = 'none';
    }
    
    /**
     * Checks the status of all cameras and updates the UI accordingly.
     * 
     * This function queries the camera status API for both CPNX and CLNX cameras
     * and updates the UI based on their current status. It handles three states:
     * "No Camera Found" (hides video, shows error message), "Running" (shows video
     * with fresh timestamp to prevent caching, hides message), and "Stopped" (hides
     * video, shows stopped message). The function makes parallel requests for both
     * cameras and handles errors gracefully.
     */
    function checkCameraStatus() {
        ['cpnx', 'clnx'].forEach(cameraType => {
            fetch(`/camera_status/${cameraType}`)
                .then(response => response.json())
                .then(data => {
                    const videoElement = document.getElementById(`${cameraType}-video`);
                    const msgElement = document.querySelector(`#${cameraType}-camera-panel .camera-stopped-msg`);
                    const infoElement = document.getElementById(`${cameraType}-info`);
                    
                    if (data.status === 'No Camera Found') {
                        if (videoElement) videoElement.style.display = 'none';
                        if (msgElement) {
                            msgElement.textContent = 'No Camera Found';
                            msgElement.style.display = 'block';
                        }
                        if (infoElement) infoElement.style.display = 'none';
                    } else if (data.status === 'Running') {
                        if (videoElement) {
                            videoElement.src = `/video_feed/${cameraType}?` + new Date().getTime();
                            videoElement.style.display = 'block';
                        }
                        if (msgElement) msgElement.style.display = 'none';
                        if (infoElement) infoElement.style.display = 'block';
                    } else {
                        if (videoElement) videoElement.style.display = 'none';
                        if (msgElement) {
                            msgElement.textContent = `${cameraType.toUpperCase()} Camera is stopped. Click 'Start Stream' to view.`;
                            msgElement.style.display = 'block';
                        }
                        if (infoElement) infoElement.style.display = 'none';
                    }
                })
                .catch(error => {
                    console.error(`Error checking ${cameraType} camera status:`, error);
                });
        });
    }
    
    /**
     * Sends a control command to the camera API (start/stop).
     * 
     * This function sends a POST request to the camera control API endpoint with
     * the specified action (start or stop) and camera type. It handles the response
     * by either updating the camera UI on success or displaying an error message
     * on failure. The function uses form-encoded data and handles network errors
     * gracefully by showing appropriate error messages to the user.
     * 
     * @param {string} action - Action to perform ('start' or 'stop')
     * @param {string} cameraType - Type of camera ('cpnx', 'clnx', or 'dual')
     */
    function controlCamera(action, cameraType) {
        fetch('/camera_control', {
            method: 'POST',
            headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
            body: `action=${action}&camera_type=${cameraType}`
        })
        .then(response => response.json())
        .then(data => {
            console.log(`Camera ${cameraType} ${action}:`, data);
            if (data.status === 'error') {
                showCameraError(cameraType, data.message || 'Failed to control camera');
            } else {
                updateCameraUI(cameraType, action);
            }
        })
        .catch(error => {
            console.error(`Error controlling ${cameraType} camera:`, error);
            showCameraError(cameraType, error.message || 'Failed to control camera');
        });
    }
    
    /**
     * Updates the camera UI based on the current camera status and action.
     * 
     * This function fetches the current camera status from the API and updates the
     * UI accordingly. It handles both single camera mode (cpnx/clnx) and dual camera
     * mode. For dual mode, it updates both camera streams independently. The function
     * updates video sources with timestamps to prevent caching, shows/hides video
     * elements and messages, updates FPS displays based on mode, and handles "No
     * Camera Found" states appropriately.
     * 
     * @param {string} cameraType - Type of camera ('cpnx', 'clnx', or 'dual')
     * @param {string} action - Action performed ('start' or 'stop')
     */
    function updateCameraUI(cameraType, action) {
        fetch(`/camera_status/${cameraType}`)
            .then(response => response.json())
            .then(data => {
                if (cameraType === 'dual') {
                    // Handle dual camera UI
                    const cpnxVideo = document.getElementById('dual-cpnx-video');
                    const clnxVideo = document.getElementById('dual-clnx-video');
                    const cpnxMsg = document.querySelector('#dual-camera-panel .dual-video-box:first-child .camera-stopped-msg');
                    const clnxMsg = document.querySelector('#dual-camera-panel .dual-video-box:last-child .camera-stopped-msg');
                    const cpnxInfo = document.getElementById('dual-cpnx-info');
                    const clnxInfo = document.getElementById('dual-clnx-info');
                    
                    if (action === 'start') {
                        // Update CPNX camera
                        if (data.status.cpnx === 'Running') {
                            if (cpnxVideo) cpnxVideo.src = CONFIG.api_endpoints.video_feed_cpnx + '?' + new Date().getTime();
                            showStream(cpnxVideo, cpnxMsg, cpnxInfo);
                        } else if (data.status.cpnx === 'No Camera Found') {
                            if (cpnxVideo) cpnxVideo.style.display = 'none';
                            if (cpnxMsg) {
                                cpnxMsg.textContent = 'No Camera Found';
                                cpnxMsg.style.display = 'block';
                            }
                            if (cpnxInfo) cpnxInfo.style.display = 'none';
                        } else {
                            showStoppedMsg(cpnxVideo, cpnxMsg, cpnxInfo);
                        }
                        
                        // Update CLNX camera
                        if (data.status.clnx === 'Running') {
                            if (clnxVideo) clnxVideo.src = CONFIG.api_endpoints.video_feed_clnx + '?' + new Date().getTime();
                            showStream(clnxVideo, clnxMsg, clnxInfo);
                        } else if (data.status.clnx === 'No Camera Found') {
                            if (clnxVideo) clnxVideo.style.display = 'none';
                            if (clnxMsg) {
                                clnxMsg.textContent = 'No Camera Found';
                                clnxMsg.style.display = 'block';
                            }
                            if (clnxInfo) clnxInfo.style.display = 'none';
                        } else {
                            showStoppedMsg(clnxVideo, clnxMsg, clnxInfo);
                        }
                        
                        updateFPSDisplays(true); // 5 FPS for dual mode
                    } else {
                        // Stop action
                        if (cpnxVideo) cpnxVideo.src = '';
                        if (clnxVideo) clnxVideo.src = '';
                        showStoppedMsg(cpnxVideo, cpnxMsg, cpnxInfo);
                        showStoppedMsg(clnxVideo, clnxMsg, clnxInfo);
                        updateFPSDisplays(false); // 30 FPS when stopped
                    }
                } else {
                    // Handle single camera UI
                    const videoElement = document.getElementById(`${cameraType}-video`);
                    const msgElement = document.querySelector(`#${cameraType}-camera-panel .camera-stopped-msg`);
                    const infoElement = document.getElementById(`${cameraType}-info`);
                    
                    if (action === 'start') {
                        if (data.status === 'No Camera Found') {
                            showCameraError(cameraType, 'No Camera Found');
                        } else if (data.status === 'Running') {
                            if (videoElement) {
                                const videoFeedEndpoint = cameraType === 'cpnx' 
                                    ? CONFIG.api_endpoints.video_feed_cpnx 
                                    : CONFIG.api_endpoints.video_feed_clnx;
                                videoElement.src = `${videoFeedEndpoint}?` + new Date().getTime();
                                showStream(videoElement, msgElement, infoElement);
                            }
                            updateFPSDisplays(false); // 30 FPS for single mode
                        } else {
                            showStoppedMsg(videoElement, msgElement, infoElement);
                        }
                    } else {
                        // Stop action
                        if (videoElement) {
                            videoElement.src = '';
                            showStoppedMsg(videoElement, msgElement, infoElement);
                        }
                        updateFPSDisplays(false); // 30 FPS when stopped
                    }
                }
            })
            .catch(error => {
                console.error(`Error updating camera UI for ${cameraType}:`, error);
            });
    }
    
    // Initialize camera status on page load
    checkCameraStatus();
    // Don't start FPS updates on page load - only start when cameras are active
    
    // Attach event listeners to camera control buttons
    const startBtns = document.querySelectorAll('.start-stream-btn');
    const stopBtns = document.querySelectorAll('.stop-stream-btn');
    
    startBtns.forEach(btn => {
        btn.addEventListener('click', () => {
            const cameraType = btn.getAttribute('data-camera');
            controlCamera('start', cameraType);
        });
    });
    
    stopBtns.forEach(btn => {
        btn.addEventListener('click', () => {
            const cameraType = btn.getAttribute('data-camera');
            controlCamera('stop', cameraType);
        });
    });
    
    // Camera sub-tab switching
    const cameraSubTabBtns = document.querySelectorAll('.camera-sub-tab-btn');
    const cameraPanels = document.querySelectorAll('.camera-panel');
    
    cameraSubTabBtns.forEach(btn => {
        btn.addEventListener('click', () => {
            const cameraType = btn.getAttribute('data-camera');
            
            cameraSubTabBtns.forEach(b => b.classList.remove('active'));
            btn.classList.add('active');
            
            cameraPanels.forEach(panel => panel.classList.remove('active'));
            document.getElementById(`${cameraType}-camera-panel`).classList.add('active');
        });
    });
    
    // ========================================================================
    // SECTION 6: CAROUSEL FUNCTIONALITY
    // ========================================================================
    
    const slides = document.querySelectorAll('.carousel-slide');
    const dots = document.querySelectorAll('.carousel-dots .dot');
    const prevBtn = document.getElementById('prevBtn');
    const nextBtn = document.getElementById('nextBtn');
    let currentSlide = 0;
    let carouselInterval;
    
    /**
     * Displays a specific slide in the carousel.
     * 
     * @param {number} index - Index of the slide to show
     */
    const showSlide = (index) => {
        slides.forEach((slide, i) => slide.classList.remove('active'));
        dots.forEach(dot => dot.classList.remove('active'));
        slides[index].classList.add('active');
        dots[index].classList.add('active');
        currentSlide = index;
    };
    
    /**
     * Advances to the next slide in the carousel.
     */
    const nextSlide = () => showSlide((currentSlide + 1) % slides.length);
    
    /**
     * Starts the automatic carousel rotation.
     */
    const startCarousel = () => {
        const carouselDelay = CONFIG.delays?.carousel_interval || 3000;
        if (!carouselInterval) {
            carouselInterval = setInterval(nextSlide, carouselDelay);
        }
    };
    
    /**
     * Stops the automatic carousel rotation.
     */
    const stopCarousel = () => {
        clearInterval(carouselInterval);
        carouselInterval = null;
    };
    
    // Carousel navigation event listeners
    if (nextBtn) {
        nextBtn.addEventListener('click', () => {
            stopCarousel();
            nextSlide();
            startCarousel();
        });
    }
    
    if (prevBtn) {
        prevBtn.addEventListener('click', () => {
            stopCarousel();
            showSlide((currentSlide - 1 + slides.length) % slides.length);
            startCarousel();
        });
    }
    
    dots.forEach((dot, index) => {
        dot.addEventListener('click', () => {
            stopCarousel();
            showSlide(index);
            startCarousel();
        });
    });
    
    // ========================================================================
    // SECTION 7: TAB NAVIGATION & LAYOUT MANAGEMENT
    // ========================================================================
    
    /**
     * Handles tab switching and manages layout states.
     */
    tabBtns.forEach(btn => {
        btn.addEventListener('click', () => {
            // Update active button and panel
            tabBtns.forEach(b => b.classList.remove('active'));
            btn.classList.add('active');
            tabPanels.forEach(panel => panel.classList.remove('active'));
            const activePanel = document.getElementById(btn.dataset.tab + '-panel');
            if (activePanel) activePanel.classList.add('active');
            
            // Manage layout states and data updates
            dashboardContainer.classList.remove('sensor-view-active', 'live-streaming-active', 'image-ops-active');
            
            if (btn.dataset.tab === 'sensor-dashboard') {
                dashboardContainer.classList.add('sensor-view-active');
                carouselContainer.style.display = 'none';
                stopCarousel();
                initAllSensorGraphs();
                startDataUpdates();
            } else if (btn.dataset.tab === 'live-streaming') {
                dashboardContainer.classList.add('live-streaming-active');
                carouselContainer.style.display = 'none';
                startCarousel();
                startDataUpdates();
            } else if (btn.dataset.tab === 'image-ops') {
                dashboardContainer.classList.add('image-ops-active');
                carouselContainer.style.display = 'none';
                stopCarousel();
                stopDataUpdates();
            } else {
                carouselContainer.style.display = 'block';
                startCarousel();
                stopDataUpdates();
            }
            
            // Show/hide exit button based on active tab
            if (btn.dataset.tab === 'sensor-dashboard' || 
                btn.dataset.tab === 'live-streaming' || 
                btn.dataset.tab === 'image-ops') {
                exitBtn.style.display = 'inline-block';
            } else {
                exitBtn.style.display = 'none';
            }
        });
    });
    
    // Exit button handler
    if (exitBtn) {
        exitBtn.style.display = 'none';
        exitBtn.addEventListener('click', () => {
            window.location.href = '/';
        });
    }
    
    // ========================================================================
    // SECTION 8: MODAL DIALOGS
    // ========================================================================
    
    // Threshold modal
    const thresholdModal = document.getElementById('threshold-modal');
    let currentMetric = null;
    
    if (thresholdModal) {
        document.querySelectorAll('.metric-config').forEach(btn => {
            btn.addEventListener('click', e => {
                e.stopPropagation();
                const metricCard = btn.closest('.metric-card');
                if (metricCard) {
                    currentMetric = metricCard.dataset.metric;
                    const metricTitle = metricCard.dataset.title || currentMetric;
                    const metricUnit = metricCard.dataset.unit || '';
                    
                    // Update modal title
                    const modalTitle = document.getElementById('modal-title');
                    if (modalTitle) {
                        modalTitle.textContent = `Configure Threshold - ${metricTitle}`;
                    }
                    
                    // Update unit display
                    const thresholdUnit = document.getElementById('safe-max-unit');
                    if (thresholdUnit) {
                        thresholdUnit.textContent = metricUnit;
                    }
                    
                    // Load current threshold or first reading
                    const thresholdInput = document.getElementById('safe-max');
                    if (thresholdInput) {
                        const currentThreshold = thresholds[currentMetric] !== undefined 
                            ? thresholds[currentMetric] 
                            : (firstReadings[currentMetric] !== undefined ? firstReadings[currentMetric] : '');
                        thresholdInput.value = currentThreshold;
                    }
                    
                    thresholdModal.classList.remove('hidden');
                }
            });
        });
        
        const modalClose = document.getElementById('modal-close');
        const modalCancel = document.getElementById('modal-cancel');
        const modalSave = document.getElementById('modal-save');
        
        if (modalClose) {
            modalClose.addEventListener('click', () => thresholdModal.classList.add('hidden'));
        }
        if (modalCancel) {
            modalCancel.addEventListener('click', () => thresholdModal.classList.add('hidden'));
        }
        if (modalSave) {
            modalSave.addEventListener('click', () => {
                if (currentMetric) {
                    const thresholdInput = document.getElementById('safe-max');
                    if (thresholdInput && thresholdInput.value !== '') {
                        const thresholdValue = parseFloat(thresholdInput.value);
                        if (!isNaN(thresholdValue)) {
                            thresholds[currentMetric] = thresholdValue;
                            showToast('Threshold saved!', 'success');
                            thresholdModal.classList.add('hidden');
                        } else {
                            showToast('Please enter a valid number', 'error');
                        }
                    } else {
                        showToast('Please enter a threshold value', 'error');
                    }
                }
            });
        }
    }
    
    // ISP settings modal
    const ispModal = document.getElementById('isp-modal');
    if (ispModal) {
        const ispSettingsBtn = document.getElementById('isp-settings-btn');
        const ispModalClose = document.getElementById('isp-modal-close');
        
        if (ispSettingsBtn) {
            ispSettingsBtn.addEventListener('click', () => ispModal.classList.remove('hidden'));
        }
        if (ispModalClose) {
            ispModalClose.addEventListener('click', () => ispModal.classList.add('hidden'));
        }
    }
    
    // Circuit viewer modal
    const circuitViewer = document.getElementById('circuit-viewer');
    if (circuitViewer) {
        const circuitViewBtn = document.getElementById('circuit-view-btn');
        const closeCircuitBtn = document.getElementById('close-circuit-btn');
        
        if (circuitViewBtn) {
            circuitViewBtn.addEventListener('click', () => circuitViewer.classList.remove('hidden'));
        }
        if (closeCircuitBtn) {
            closeCircuitBtn.addEventListener('click', () => circuitViewer.classList.add('hidden'));
        }
    }
    
    // ========================================================================
    // SECTION 9: IMAGE OPERATIONS
    // ========================================================================
    
    const captureImageBtn = document.getElementById('capture-image-btn');
    const saveImageBtn = document.getElementById('save-image-btn');
    const capturedImage = document.getElementById('captured-image');
    const imagePlaceholder = document.getElementById('image-placeholder');
    const imageSavePath = document.getElementById('image-save-path');
    const imageFormatSelect = document.getElementById('image-format-select');
    const saveLocationRadios = document.querySelectorAll('input[name="save-location"]');
    const browseFolderBtn = document.getElementById('browse-folder-btn');
    
    /**
     * Gets the currently selected save location (client or server).
     * 
     * This function queries the DOM to find the currently selected radio button
     * for save location. It returns 'client' if the client option is selected,
     * 'server' if the server option is selected, or defaults to 'client' if
     * no radio button is selected.
     * 
     * @returns {string} 'client' or 'server'
     */
    function getSaveLocation() {
        const selectedRadio = document.querySelector('input[name="save-location"]:checked');
        return selectedRadio ? selectedRadio.value : 'client';
    }
    
    /**
     * Loads saved paths from localStorage based on current save location.
     * 
     * This function retrieves previously saved save paths from localStorage for
     * both client and server locations. It then populates the image save path
     * input field with the appropriate saved path based on the currently selected
     * save location (client or server). If no saved path exists, it uses the
     * default path from configuration.
     */
    function loadSavedPaths() {
        const saveLocation = getSaveLocation();
        const clientKey = CONFIG.local_storage_keys.client_save_path;
        const serverKey = CONFIG.local_storage_keys.server_save_path;
        const savedClientPath = localStorage.getItem(clientKey);
        const savedServerPath = localStorage.getItem(serverKey);
        
        if (saveLocation === 'server') {
            imageSavePath.value = savedServerPath || CONFIG.paths.default_server_save_path;
        } else {
            imageSavePath.value = savedClientPath || CONFIG.paths.default_client_save_path;
        }
    }
    
    /**
     * Saves a path to localStorage.
     * 
     * This function persists the user's selected save path to browser localStorage
     * so it can be restored on subsequent visits. It uses different storage keys
     * for client and server paths, allowing users to have separate preferences
     * for each save location type.
     * 
     * @param {string} path - Path to save
     * @param {boolean} isServer - Whether this is a server path
     */
    function savePathToStorage(path, isServer) {
        if (isServer) {
            localStorage.setItem(CONFIG.local_storage_keys.server_save_path, path);
        } else {
            localStorage.setItem(CONFIG.local_storage_keys.client_save_path, path);
        }
    }
    
    /**
     * Saves a Blob to the client's file system.
     * 
     * This function attempts to save a file using the File System Access API if
     * a directory has been selected by the user. It creates an 'edgeHUB' subfolder
     * in the selected directory and writes the blob directly to that location.
     * If the File System Access API is not available or fails, it falls back to
     * the traditional browser download mechanism, which saves to the Downloads
     * folder. Returns true if saved via File System API, 'fallback' if using
     * browser download, or false on error.
     * 
     * @param {Blob} blob - Blob to save
     * @param {string} filename - Filename for the saved file
     * @returns {Promise<boolean|string>} true on success, 'fallback' if fallback used, false on error
     */
    async function saveBlobToClient(blob, filename) {
        try {
            if (selectedDirectoryHandle) {
                try {
                    const saveFolderName = CONFIG.paths.save_folder_name || 'edgeHUB';
                    const edgeHUBHandle = await selectedDirectoryHandle.getDirectoryHandle(saveFolderName, { create: true });
                    
                    const fileHandle = await edgeHUBHandle.getFileHandle(filename, { create: true });
                    const writable = await fileHandle.createWritable();
                    await writable.write(blob);
                    await writable.close();
                    
                    return true;
                } catch (error) {
                    console.log('Error using saved directory handle, falling back to regular download:', error);
                    selectedDirectoryHandle = null;
                }
            }
            
            const url = URL.createObjectURL(blob);
            const link = document.createElement('a');
            link.href = url;
            link.download = filename;
            document.body.appendChild(link);
            link.click();
            document.body.removeChild(link);
            URL.revokeObjectURL(url);
            
            return 'fallback';
        } catch (error) {
            const errorMsg = CONFIG.errors?.error_downloading_image || 'Error downloading file:';
            console.error(errorMsg, error);
            return false;
        }
    }
    
    /**
     * Downloads an image to the client's file system.
     * 
     * This function converts base64-encoded image data into a Blob object and then
     * saves it using saveBlobToClient(). It handles format detection if not provided,
     * defaulting to PNG format. The function decodes the base64 string character by
     * character into a byte array, creates a Blob with the appropriate MIME type,
     * and delegates the actual file saving to saveBlobToClient().
     * 
     * @param {string} imageData - Base64 encoded image data
     * @param {string} filename - Filename for the saved image
     * @param {Object} imageFormat - Format object with mimeType and extension
     * @returns {Promise<boolean|string>} true on success, 'fallback' if fallback used, false on error
     */
    async function downloadImageToClient(imageData, filename, imageFormat = null) {
        try {
            let format = imageFormat;
            if (!format) {
                format = { mimeType: 'image/png', extension: 'png' };
            }
            
            const byteCharacters = atob(imageData);
            const byteNumbers = new Array(byteCharacters.length);
            for (let i = 0; i < byteCharacters.length; i++) {
                byteNumbers[i] = byteCharacters.charCodeAt(i);
            }
            const byteArray = new Uint8Array(byteNumbers);
            const blob = new Blob([byteArray], { type: format.mimeType });
            
            return await saveBlobToClient(blob, filename);
        } catch (error) {
            const errorMsg = CONFIG.errors?.error_downloading_image || 'Error downloading image:';
            console.error(errorMsg, error);
            return false;
        }
    }
    
    /**
     * Retrieves image data from various sources for download.
     * 
     * This function attempts to retrieve image data from multiple sources in
     * priority order. First, it tries to fetch the image from the blob URL if
     * the captured image is displayed from a blob URL (not a data URL). It
     * fetches the blob, converts it to base64, and returns the data. If that
     * fails or is not available, it falls back to using the stored currentImageData
     * variable which contains base64-encoded image data. Returns null if no
     * image data can be found from any source.
     * 
     * @returns {Promise<Object|null>} Object with imageData and format, or null if not found
     */
    async function getImageDataForDownload() {
        const format = { mimeType: 'image/png', extension: 'png' };
        
        if (capturedImage.src && !capturedImage.src.startsWith('data:')) {
            try {
                const imageUrl = capturedImage.src.split('?')[0];
                const response = await fetch(imageUrl);
                if (response.ok) {
                    const blob = await response.blob();
                    return new Promise((resolve, reject) => {
                        const reader = new FileReader();
                        reader.onloadend = () => {
                            const base64 = reader.result.split(',')[1];
                            resolve({ imageData: base64, format: format });
                        };
                        reader.onerror = reject;
                        reader.readAsDataURL(blob);
                    });
                }
            } catch (error) {
                const errorMsg = CONFIG.errors?.error_fetching_image || 'Error fetching image from URL:';
                console.error(errorMsg, error);
            }
        }
        
        if (currentImageData) {
            return { imageData: currentImageData, format: format };
        }
        
        return null;
    }
    
    if (saveLocationRadios.length > 0) {
        saveLocationRadios.forEach(radio => {
            radio.addEventListener('change', (e) => {
                const isServer = e.target.value === 'server';
                
                if (isServer) {
                    const savedServerPath = localStorage.getItem(CONFIG.local_storage_keys.server_save_path);
                    imageSavePath.value = savedServerPath || CONFIG.paths.default_server_save_path;
                } else {
                    const savedClientPath = localStorage.getItem(CONFIG.local_storage_keys.client_save_path);
                    imageSavePath.value = savedClientPath || CONFIG.paths.default_client_save_path;
                }
            });
        });
    }
    
    loadSavedPaths();
    
    if (CONFIG.cameras) {
        const cpnxTitle = document.getElementById('cpnx-camera-title');
        const clnxTitle = document.getElementById('clnx-camera-title');
        
        if (cpnxTitle && CONFIG.cameras.pi) {
            cpnxTitle.textContent = CONFIG.cameras.pi.display_name || 'MIPI Passthrough CPNX Pi Camera Feed';
        }
        if (clnxTitle && CONFIG.cameras.usb) {
            clnxTitle.textContent = CONFIG.cameras.usb.display_name || 'MIPI Passthrough CLNX USB Camera Feed';
        }
    }
    
    if (CONFIG.cameras) {
        const resolutionElements = [
            { id: 'cpnx-resolution', camera: CONFIG.cameras.pi },
            { id: 'clnx-resolution', camera: CONFIG.cameras.usb },
            { id: 'dual-cpnx-resolution', camera: CONFIG.cameras.pi },
            { id: 'dual-clnx-resolution', camera: CONFIG.cameras.usb },
            { id: 'clnx-resolution-info', camera: CONFIG.cameras.usb }
        ];
        
        resolutionElements.forEach(({ id, camera }) => {
            const element = document.getElementById(id);
            if (element && camera) {
                element.textContent = `${camera.width}×${camera.height}`;
            }
        });
    }
    
    const saveInfoText = document.getElementById('save-info-text');
    if (saveInfoText && CONFIG.messages && CONFIG.messages.info_text) {
        saveInfoText.innerHTML = CONFIG.messages.info_text;
    }
    
    if (browseFolderBtn) {
        browseFolderBtn.addEventListener('click', async () => {
            const isServer = getSaveLocation() === 'server';
            
            if (isServer) {
                showToast(CONFIG.messages.server_path_manual || 'For server paths, please enter the path manually. Server file system cannot be browsed from browser.', 'info');
            } else {
                if ('showDirectoryPicker' in window) {
                    try {
                        const dirHandle = await window.showDirectoryPicker({ mode: 'readwrite' });
                        selectedDirectoryHandle = dirHandle;
                        
                        let displayPath = dirHandle.name || 'Selected folder';
                        const defaultClientPath = CONFIG.paths.default_client_save_path;
                        if (dirHandle.name === defaultClientPath || displayPath.toLowerCase().includes('download')) {
                            displayPath = defaultClientPath;
                        }
                        
                        imageSavePath.value = displayPath;
                        savePathToStorage(displayPath, false);
                        
                        showToast(CONFIG.messages.folder_selected || 'Folder selected. Images will be saved in edgeHUB subfolder.', 'success');
                    } catch (error) {
                        if (error.name !== 'AbortError') {
                            console.error('Error selecting folder:', error);
                            showToast('Error selecting folder. Please enter path manually.', 'error');
                        }
                    }
                } else {
                    showToast('Folder picker not supported in this browser. Please enter path manually.', 'warning');
                }
            }
        });
    }
    
    if (imageSavePath) {
        imageSavePath.addEventListener('blur', () => {
            const path = imageSavePath.value.trim();
            if (path) {
                const isServer = getSaveLocation() === 'server';
                savePathToStorage(path, isServer);
            }
        });
    }
    
    /**
     * Converts .bin file (RGB_PLANAR format) to PNG blob for display
     * @param {ArrayBuffer} binData - Raw binary data
     * @param {number} width - Image width
     * @param {number} height - Image height
     * @returns {Promise<Blob>} PNG blob
     */
    /**
     * Converts raw binary image data to a PNG Blob.
     * 
     * This function takes raw binary image data (typically from a camera sensor)
     * and converts it into a PNG-format Blob that can be displayed or saved.
     * It creates a canvas element, converts the binary data to an ImageData object,
     * draws it on the canvas, and then converts the canvas to a PNG Blob using
     * the toBlob() method. The function handles the asynchronous nature of canvas
     * conversion by returning a Promise.
     * 
     * @param {ArrayBuffer|Uint8Array} binData - Raw binary image data
     * @param {number} width - Width of the image in pixels
     * @param {number} height - Height of the image in pixels
     * @returns {Promise<Blob>} PNG-format Blob of the converted image
     */
    /**
     * Converts raw binary image data to a PNG Blob.
     * 
     * This function takes raw binary image data (typically from a camera sensor)
     * and converts it into a PNG-format Blob that can be displayed or saved.
     * It creates a canvas element, converts the binary data to an ImageData object,
     * draws it on the canvas, and then converts the canvas to a PNG Blob using
     * the toBlob() method. The function handles the asynchronous nature of canvas
     * conversion by returning a Promise.
     * 
     * @param {ArrayBuffer|Uint8Array} binData - Raw binary image data
     * @param {number} width - Width of the image in pixels
     * @param {number} height - Height of the image in pixels
     * @returns {Promise<Blob>} PNG-format Blob of the converted image
     */
    async function convertBinToPngBlob(binData, width, height) {
        const canvas = document.createElement('canvas');
        canvas.width = width;
        canvas.height = height;
        const ctx = canvas.getContext('2d');
        const imageData = ctx.createImageData(width, height);
        
        const numPixels = width * height;
        const dataView = new DataView(binData);
        
        for (let i = 0; i < numPixels; i++) {
            const r = dataView.getUint8(i);
            const g = dataView.getUint8(numPixels + i);
            const b = dataView.getUint8(2 * numPixels + i);
            
            const pixelIndex = i * 4;
            imageData.data[pixelIndex] = r;
            imageData.data[pixelIndex + 1] = g;
            imageData.data[pixelIndex + 2] = b;
            imageData.data[pixelIndex + 3] = 255;
        }
        
        ctx.putImageData(imageData, 0, 0);
        
        return new Promise((resolve) => {
            canvas.toBlob((blob) => {
                resolve(blob);
            }, 'image/png');
        });
    }
    
    // Capture Image Handler
    if (captureImageBtn) {
        captureImageBtn.addEventListener('click', async () => {
            try {
                captureImageBtn.disabled = true;
                captureImageBtn.innerHTML = '<i class="fas fa-spinner fa-spin"></i> Capturing...';
                
                const response = await fetch(CONFIG.api_endpoints.capture_image + '?format=bin', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' }
                });
                
                if (response.ok) {
                    const contentType = response.headers.get('content-type');
                    
                    if (contentType === 'application/octet-stream' || contentType?.includes('octet-stream')) {
                        const binBlob = await response.blob();
                        const binArrayBuffer = await binBlob.arrayBuffer();
                        
                        const width = parseInt(response.headers.get('X-Image-Width') || '384');
                        const height = parseInt(response.headers.get('X-Image-Height') || '288');
                        
                        currentBinData = binArrayBuffer;
                        
                        const pngBlob = await convertBinToPngBlob(binArrayBuffer, width, height);
                        const imageUrl = URL.createObjectURL(pngBlob);
                        
                        const reader = new FileReader();
                        reader.onloadend = () => {
                            currentImageData = reader.result.split(',')[1];
                        };
                        reader.readAsDataURL(pngBlob);
                        
                        capturedImage.src = imageUrl;
                        capturedImage.style.display = 'block';
                        imagePlaceholder.style.display = 'none';
                        
                        showToast(CONFIG.messages?.image_captured_success || 'Image captured successfully!', 'success');
                    } else if (contentType?.includes('image')) {
                        const blob = await response.blob();
                        const imageUrl = URL.createObjectURL(blob);
                        
                        capturedImage.src = imageUrl;
                        capturedImage.style.display = 'block';
                        imagePlaceholder.style.display = 'none';
                        
                        const reader = new FileReader();
                        reader.onloadend = () => {
                            currentImageData = reader.result.split(',')[1];
                        };
                        reader.readAsDataURL(blob);
                        
                        currentBinData = null;
                        
                        showToast(CONFIG.messages?.image_captured_success || 'Image captured successfully!', 'success');
                    } else {
                        // Response is JSON error
                        const data = await response.json();
                        showToast(data.message || CONFIG.messages?.image_capture_failed || 'Failed to capture image', 'error');
                    }
                } else {
                    // Response is JSON error
                    const data = await response.json();
                    showToast(data.message || CONFIG.messages?.image_capture_failed || 'Failed to capture image', 'error');
                }
            } catch (error) {
                const errorMsg = CONFIG.errors?.error_capturing_image_js || 'Error capturing image:';
                console.error(errorMsg, error);
                showToast(CONFIG.messages?.error_capturing_image || 'Error capturing image', 'error');
            } finally {
                captureImageBtn.disabled = false;
                captureImageBtn.innerHTML = '<i class="fas fa-camera"></i> Capture Image';
            }
        });
    }
    
    // Save Image Handler
    if (saveImageBtn) {
        saveImageBtn.addEventListener('click', async () => {
            if (!currentImageData && !capturedImage.src) {
                showToast('Please capture an image first', 'warning');
                return;
            }
            
            const saveOnServer = getSaveLocation() === 'server';
            const classification = document.querySelector('input[name="anomaly-status"]:checked')?.value || 'normal';
            
            try {
                saveImageBtn.disabled = true;
                saveImageBtn.innerHTML = '<i class="fas fa-spinner fa-spin"></i> Saving...';
                
                if (saveOnServer) {
                    // Save on server
                    let savePath = imageSavePath.value.trim();
                    if (!savePath) {
                        savePath = CONFIG.paths.default_server_save_path;
                        imageSavePath.value = savePath;
                        savePathToStorage(savePath, true);
                    }
                    
                    const imageFormat = imageFormatSelect ? imageFormatSelect.value : 'png';
                    
                    // For bin format, use raw bin data if available
                    let imageDataToSend = null;
                    if (imageFormat === 'bin' && currentBinData) {
                        // Convert ArrayBuffer to base64 for bin format using FileReader (handles large arrays correctly)
                        try {
                            const blob = new Blob([currentBinData], { type: 'application/octet-stream' });
                            imageDataToSend = await new Promise((resolve, reject) => {
                                const reader = new FileReader();
                                reader.onloadend = () => {
                                    resolve(reader.result.split(',')[1]); // Extract base64 without data URL prefix
                                };
                                reader.onerror = reject;
                                reader.readAsDataURL(blob);
                            });
                        } catch (error) {
                            console.error('Error converting bin data to base64:', error);
                            showToast('Error preparing RAW image data for save', 'error');
                            return;
                        }
                    } else {
                        // Get image data as base64 (convert from blob URL if needed)
                        imageDataToSend = currentImageData;
                        if (!imageDataToSend && capturedImage.src) {
                            if (capturedImage.src.startsWith('blob:')) {
                                // Convert blob URL to base64
                                try {
                                    const response = await fetch(capturedImage.src);
                                    const blob = await response.blob();
                                    imageDataToSend = await new Promise((resolve, reject) => {
                                        const reader = new FileReader();
                                        reader.onloadend = () => {
                                            resolve(reader.result.split(',')[1]);
                                        };
                                        reader.onerror = reject;
                                        reader.readAsDataURL(blob);
                                    });
                                } catch (error) {
                                    console.error('Error converting blob to base64:', error);
                                    showToast('Error preparing image for save', 'error');
                                    return;
                                }
                            }
                        }
                    }
                    
                    if (!imageDataToSend) {
                        showToast('No image data available to save', 'error');
                        return;
                    }
                    
                    const response = await fetch(CONFIG.api_endpoints.save_image, {
                        method: 'POST',
                        headers: { 'Content-Type': 'application/json' },
                        body: JSON.stringify({
                            image_data: imageDataToSend,
                            save_path: savePath,
                            classification: classification,
                            image_format: imageFormat,
                            save_on_server: true
                        })
                    });
                    
                    const data = await response.json();
                    
                    if (data.status === 'success') {
                        const savedPath = data.saved_path || savePath;
                        const filename = savedPath.split('/').pop() || savedPath;
                        showToast(`Image saved on server: ${filename}`, 'success');
                    } else {
                        showToast(data.message || 'Failed to save image', 'error');
                    }
                } else {
                    // Download to client
                    const selectedFormat = imageFormatSelect ? imageFormatSelect.value : null;
                    
                    // Handle bin format for client download
                    if (selectedFormat === 'bin' && currentBinData) {
                        const timestampFormat = CONFIG.image_ops.timestamp_format || 'YYYY-MM-DDTHH-mm-ss';
                        const timestamp = new Date().toISOString().replace(/[:.]/g, '-').slice(0, -5);
                        const filenamePrefix = CONFIG.image_ops.filename_prefix || 'edgehub_imageops';
                        const filename = `${filenamePrefix}_${classification}_${timestamp}.bin`;
                        
                        // Convert ArrayBuffer to Blob and save using File System Access API
                        const blob = new Blob([currentBinData], { type: 'application/octet-stream' });
                        const result = await saveBlobToClient(blob, filename);
                        
                        if (result === true) {
                            const saveFolderName = CONFIG.paths.save_folder_name || 'edgeHUB';
                            showToast(CONFIG.messages.image_saved_to_edgehub?.replace('{filename}', filename) || `Image saved to ${saveFolderName}/: ${filename}`, 'success');
                        } else if (result === 'fallback') {
                            showToast(CONFIG.messages.image_downloaded_to_downloads || `Image downloaded to Downloads. Please move to Downloads/${CONFIG.paths.save_folder_name || 'edgeHUB'}/ folder.`, 'info');
                        } else if (result === false) {
                            showToast(CONFIG.messages?.download_cancelled || 'Download cancelled', 'warning');
                        }
                    } else {
                        // Regular image format download
                        const imageDataResult = await getImageDataForDownload();
                        if (imageDataResult && imageDataResult.imageData) {
                            const detectedFormat = imageDataResult.format || { mimeType: 'image/png', extension: 'png' };
                            const imageFormat = selectedFormat || detectedFormat.extension || CONFIG.image_ops.image_format || 'png';
                            
                            const timestampFormat = CONFIG.image_ops.timestamp_format || 'YYYY-MM-DDTHH-mm-ss';
                            const timestamp = new Date().toISOString().replace(/[:.]/g, '-').slice(0, -5);
                            const filenamePrefix = CONFIG.image_ops.filename_prefix || 'edgehub_imageops';
                            const filename = `${filenamePrefix}_${classification}_${timestamp}.${imageFormat}`;
                            
                            const result = await downloadImageToClient(imageDataResult.imageData, filename, detectedFormat);
                            if (result === true) {
                                const saveFolderName = CONFIG.paths.save_folder_name || 'edgeHUB';
                                showToast(CONFIG.messages.image_saved_to_edgehub?.replace('{filename}', filename) || `Image saved to ${saveFolderName}/: ${filename}`, 'success');
                            } else if (result === 'fallback') {
                                showToast(CONFIG.messages.image_downloaded_to_downloads || `Image downloaded to Downloads. Please move to Downloads/${CONFIG.paths.save_folder_name || 'edgeHUB'}/ folder.`, 'info');
                            } else if (result === false) {
                                showToast(CONFIG.messages?.download_cancelled || 'Download cancelled', 'warning');
                            }
                        } else {
                            showToast(CONFIG.messages?.could_not_retrieve_image || 'Could not retrieve image data for download', 'error');
                        }
                    }
                }
            } catch (error) {
                console.error('Error saving image:', error);
                showToast('Error saving image', 'error');
            } finally {
                saveImageBtn.disabled = false;
                saveImageBtn.innerHTML = '<i class="fas fa-save"></i> Save Image';
            }
        });
    }
    
    // ========================================================================
    // SECTION 10: UTILITY FUNCTIONS
    // ========================================================================
    
    /**
     * Displays a toast notification message to the user.
     * 
     * This function displays a temporary notification message (toast) to inform
     * the user of various events such as success, errors, warnings, or info messages.
     * It first checks if a global showToast function exists (which may be provided
     * by a toast library), and if not, it creates a simple DOM-based toast
     * implementation. The toast appears at the bottom of the screen, auto-dismisses
     * after a few seconds, and supports different types (success, error, warning, info)
     * with corresponding colors and icons.
     * 
     * @param {string} message - The message text to display
     * @param {string} type - Type of toast: 'success', 'error', 'warning', or 'info' (default: 'info')
     */
    function showToast(message, type = 'info') {
        if (typeof window.showToast === 'function') {
            window.showToast(message, type);
        } else {
            const toast = document.createElement('div');
            toast.className = `toast-notification toast-${type}`;
            
            const iconMap = {
                success: 'check-circle',
                error: 'exclamation-circle',
                warning: 'exclamation-triangle',
                info: 'info-circle'
            };
            
            toast.innerHTML = `
                <i class="fas fa-${iconMap[type] || 'info-circle'}"></i>
                <span>${message}</span>
            `;
            document.body.appendChild(toast);
            
            const showDelay = CONFIG.delays?.toast_show_delay || 10;
            const hideDelay = CONFIG.delays?.toast_hide_delay || 3000;
            const removeDelay = CONFIG.delays?.toast_remove_delay || 300;
            
            setTimeout(() => toast.classList.add('show'), showDelay);
            setTimeout(() => {
                toast.classList.remove('show');
                setTimeout(() => toast.remove(), removeDelay);
            }, hideDelay);
        }
    }
    
    // ========================================================================
    // SECTION 11: INITIALIZATION
    // ========================================================================
    
    // Set initial tab state
    const initialTab = document.querySelector('.tab-btn[data-tab="sensor-dashboard"]');
    if (initialTab) {
        initialTab.classList.add('active');
    }
    const initialPanel = document.getElementById('sensor-dashboard-panel');
    if (initialPanel) {
        initialPanel.style.display = 'block';
    }
    
    dashboardContainer.classList.remove('sensor-view-active');
    if (carouselContainer) {
        carouselContainer.style.display = 'block';
    }
    startCarousel();
    
    // Start data updates if sensor dashboard or live streaming is active
    if (dashboardContainer.classList.contains('sensor-view-active') || 
        dashboardContainer.classList.contains('live-streaming-active')) {
        startDataUpdates();
    }
    
    // Handle window resizing to keep plots responsive
    window.addEventListener('resize', () => {
        if (dashboardContainer.classList.contains('sensor-view-active')) {
            ['plot-temp', 'plot-voltage', 'plot-current', 'plot-power'].forEach(id => {
                Plotly.Plots.resize(id);
            });
        }
    });
});
