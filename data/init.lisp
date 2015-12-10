;; (create-playlist 'p1)
;; (create-playlist 'p2)
;; (create-layout 'l1 t)
;; (layout-add-component 'l1 'p1 -0.1 -0.9 -0.5)
;; (layout-add-splitter 'l1)
;; (layout-add-component 'l1 'p2 -0.1 -0.9 -0.5)
;; (create-layout 'l2 nil)
;; (create-interpreter 'int1)
;; (layout-add-component 'l2 'int1 50.0 100.0 50.0)
;; (layout-add-splitter 'l2)
;; (layout-add-component 'l2 'l1 -0.1 -1.0 -0.9)
;;
;; (set-main-component 'l2)
;;
;; (create-tabs 'tab 'bottom)
;; (create-playlist 'plpl)
;;
;; (defun on-playlist-click (item-str)
;;   (playback-set-file item-str)
;;   (slider-range 'slider 0.0 (playback-length))
;;   (playback-start) )
;;
;; (defun on-slider-up (time)
;;   (playback-seek time))
;;
;; (defun on-update-slider ()
;;   (slider-value 'slider (playback-get-pos)))
;;
;; (create-timer 'update-slider 'on-update-slider)
;; (start-timer 'update-slider 500) ;; start-stop?
;;
;; (bind-mouse-double-click 'plpl 'on-playlist-click '(selected-row))
;;
;; (layout-add-component 'l2 'tab 300.0 300.0 300.0)
;; (defvar cl-red |1.0 0.0 0.0 1.0|)
;; (tabs-add-component 'tab 'plpl "First tab" cl-red)
;; (create-audio-settings 'sss)
;; (tabs-add-component 'tab 'sss "Audio Settings" |0.0 0.8 0.0 0.5|)
;; (defun on-play-clicked ()
;;   (layout-remove-splitter 'l1 0)
;;   (refresh-interface))
;;
;; ;; (defun playback-changed ()
;; ;;   (if (not (playback-is-playing))
;; ;;       (on-play-clicked)))
;; ;; (bind-playback 'playback-changed)
;;
;; (tabs-add-component 'tab (create-text-button 'playb "Play" "Plays selected track") "Playback API" |0.0 0.1 0.5 0.9|)
;; (tabs-add-component 'tab (create-slider 'slider) "Slider" |0.0 0.5 0.5 0.9|)
;; (bind-mouse-click 'playb 'on-play-clicked)
;; (bind-slider-drag-end 'slider 'on-slider-up '(slider-value))
;;
;; (refresh-interface)

(defun create-playlist-tabs ()
  (create-tabs 'playlist-tabs 'top)
  (create-playlist 'playlist)
  (tabs-add-component 'playlist-tabs 'playlist "all" |0.5 0.5 0.5 0.9|)
  'playlist-tabs)

(defun create-button-page ()
  (create-layout 'l-buttons t)
  (create-text-button 'b-play ">" "Play")
  (create-text-button 'b-pause "||" "Pause")
  (create-text-button 'b-stop "[ ]" "Stop playback")
  (create-text-button 'b-options "Options" "Audio options")
  (create-text-button 'b-interpreter "Interpreter" "GLISP Interpreter")
  (create-slider 'sl-seek)
  (create-slider 'sl-gain)
  (slider-range 'sl-gain 0.0 1.0)
  (slider-value 'sl-gain 1.0)
  (layout-add-component 'l-buttons 'b-play 30.0 30.0 30.0)
  (layout-add-component 'l-buttons 'b-pause 30.0 30.0 30.0)
  (layout-add-component 'l-buttons 'b-stop 30.0 30.0 30.0)
  (layout-add-component 'l-buttons 'b-options 80.0 80.0 80.0)
  (layout-add-component 'l-buttons 'b-interpreter 80.0 80.0 80.0)
  (layout-add-component 'l-buttons 'sl-seek -0.1 -1.0 -1.0)
  (layout-add-component 'l-buttons 'sl-gain 200.0 200.0 200.0)
  'l-buttons)

(create-layout 'l-main nil)
(layout-add-component 'l-main (create-button-page) 20.0 20.0 20.0)
(layout-add-component 'l-main (create-playlist-tabs) -1.0 -1.0 -1.0)

;; callbacks
(defun get-audio-settings ()
  (if (has-component 'audio-settings)
      'audio-settings
    (create-audio-settings 'audio-settings)))

(defun spawn-audio-options ()
  (if (not (tabs-index 'playlist-tabs "Audio Options"))
      (progn
	(tabs-add-component 'playlist-tabs (get-audio-settings) "Audio Options" |0.9 0.5 0.5 0.9|)
	(tabs-index 'playlist-tabs "Audio Options"))))

(defun get-interpreter ()
  (if (has-component 'interpreter)
      'interpreter
    (create-interpreter 'interpreter)))

(defun spawn-interpreter ()
  (if (not (tabs-index 'playlist-tabs "Interpreter"))
      (progn
	(tabs-add-component 'playlist-tabs (get-interpreter) "Interpreter" |0.5 0.9 0.5 0.9|)
	(tabs-index 'playlist-tabs "Interpreter"))))

(defun on-stop ()
  (playback-stop)
  (playback-seek 0.0))

(defun on-pause ()
  (playback-stop))

(defun on-play ()
  ;; TODO: fetch from playlist
  (playback-start))

(defun on-playlist-click (item-str)
  (playback-set-file item-str)
  (slider-range 'sl-seek 0.0 (playback-length))
  (playback-start))

(defun playback-changed ()
  (if (playback-is-playing)
      (start-timer 'update-slider 100)
    (stop-timer 'update-slider)))

(defun on-slider-up (time)
  (playback-seek time))

(defun on-update-slider ()
  (slider-value 'sl-seek (playback-get-pos)))

(defun on-slider-gain (new-gain)
  (playback-gain new-gain))

;; bindings
(bind-mouse-click 'b-options 'spawn-audio-options)
(bind-mouse-click 'b-interpreter 'spawn-interpreter)
(bind-mouse-click 'b-play 'on-play)
(bind-mouse-click 'b-pause 'on-pause)
(bind-mouse-click 'b-stop 'on-stop)
(bind-mouse-double-click 'playlist 'on-playlist-click '(selected-row))
(bind-slider-drag-end 'sl-seek 'on-slider-up '(slider-value))
(bind-slider-changed 'sl-gain 'on-slider-gain '(slider-value))
(create-timer 'update-slider 'on-update-slider)
(bind-playback 'playback-changed)

(set-main-component 'l-main)
(refresh-interface)
