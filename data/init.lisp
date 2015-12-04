(create-playlist 'p1)
(create-playlist 'p2)
(create-layout 'l1 t)
(layout-add-component 'l1 'p1 -0.1 -0.9 -0.5)
(layout-add-splitter 'l1)
(layout-add-component 'l1 'p2 -0.1 -0.9 -0.5)
(create-layout 'l2 nil)
(create-interpreter 'int1)
(layout-add-component 'l2 'int1 50.0 100.0 50.0)
(layout-add-splitter 'l2)
(layout-add-component 'l2 'l1 -0.1 -1.0 -0.9)

(set-main-component 'l2)

(create-tabs 'tab 'bottom)
(create-playlist 'plpl)

(defun on-playlist-click (item-str)
  (playback-set-file item-str)
  (slider-set-range 'slider 0.0 (playback-length))
  (playback-start) )

(defun on-slider-up (time)
  (playback-seek time))

(bind-mouse-double-click 'plpl 'on-playlist-click '(selected-row))

(layout-add-component 'l2 'tab 300.0 300.0 300.0)
(defvar cl-red |1.0 0.0 0.0 1.0|)
(tabs-add-component 'tab 'plpl "First tab" cl-red)
(create-audio-settings 'sss)
(tabs-add-component 'tab 'sss "Audio Settings" |0.0 0.8 0.0 0.5|)
(defun on-play-clicked ()
  (layout-remove-splitter 'l1 0)
  (refresh-interface))

(tabs-add-component 'tab (create-text-button 'playb "Play" "Plays selected track") "Playback API" |0.0 0.1 0.5 0.9|)
(tabs-add-component 'tab (create-slider 'slider) "Slider" |0.0 0.5 0.5 0.9|)
(bind-mouse-click 'playb 'on-play-clicked)
(bind-slider-drag-end 'slider 'on-slider-up '(slider-value))

(refresh-interface)
