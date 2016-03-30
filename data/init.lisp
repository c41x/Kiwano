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

(defvar current-id "")
(defvar current-index 0)
(defvar current-playlist nil)

;; for given id returns new playlist
(defun init-playlist (id)
  ;;(message-box (strs "creating playlist: " id) "asdf")
  (create-playlist id)
  (playlist-add-column id "track" 'track 50 20 1000)
  (playlist-add-column id "album" 'album 200 150 1000)
  (playlist-add-column id "artist" 'artist 200 150 1000)
  (playlist-add-column id "title" 'title 200 150 1000)
  (playlist-add-column id "year" 'year 70 50 1000)
  (playlist-add-column id "count" 'aaaaaaaaaaaaa 50 20 1000)
  (bind-mouse-double-click id 'on-playlist-click '(selected-row
						   selected-row-id
						   selected-row-index
						   component-name))
  id)

(defun create-playlist-tabs ()
  (create-tabs 'playlist-tabs 'top)
  ;;(tabs-add-component 'playlist-tabs (init-playlist 'playlist)
  ;;"all" |0.5 0.5 0.5 0.9|)
  'playlist-tabs)

(defun create-button-page ()
  (create-layout 'l-buttons t)
  (create-text-button 'b-play ">" "Play")
  (create-text-button 'b-pause "||" "Pause")
  (create-text-button 'b-stop "[ ]" "Stop playback")
  (create-text-button 'b-prev "<<" "Previous")
  (create-text-button 'b-next ">>" "Next")
  (create-text-button 'b-rand "%" "Random")
  (create-text-button 'b-options "Options" "Audio options")
  (create-text-button 'b-interpreter "Interpreter" "GLISP Interpreter")
  (create-text-button 'b-new-playlist "New Playlist" "Create new playlist")
  (create-slider 'sl-seek)
  (component-enabled 'sl-seek nil)
  (create-slider 'sl-gain)
  (slider-range 'sl-gain 0.0 1.0)
  (slider-value 'sl-gain 1.0)
  (layout-add-component 'l-buttons 'b-play 30.0 30.0 30.0)
  (layout-add-component 'l-buttons 'b-pause 30.0 30.0 30.0)
  (layout-add-component 'l-buttons 'b-stop 30.0 30.0 30.0)
  (layout-add-component 'l-buttons 'b-prev 30.0 30.0 30.0)
  (layout-add-component 'l-buttons 'b-next 30.0 30.0 30.0)
  (layout-add-component 'l-buttons 'b-rand 30.0 30.0 30.0)
  (layout-add-component 'l-buttons 'b-options 80.0 80.0 80.0)
  (layout-add-component 'l-buttons 'b-interpreter 80.0 80.0 80.0)
  (layout-add-component 'l-buttons 'b-new-playlist 80.0 80.0 80.0)
  (layout-add-component 'l-buttons 'sl-seek -0.1 -1.0 -1.0)
  (layout-add-component 'l-buttons 'sl-gain 100.0 100.0 100.0)
  'l-buttons)

(create-layout 'l-main nil)
(layout-add-component 'l-main (create-button-page) 20.0 20.0 20.0)
(layout-add-component 'l-main (create-playlist-tabs) -1.0 -1.0 -1.0)

;; callbacks
(defun spawn-audio-options ()
  (if (not (tabs-index 'playlist-tabs "Audio Options"))
      (progn
	(tabs-add-component 'playlist-tabs (audio-settings) "Audio Options" |0.9 0.5 0.5 0.9|)
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

(defun on-new-playlist ()
  (tabs-add-component 'playlist-tabs
		      (init-playlist (unique-id "playlist"))
		      (input-box "Playlist Name" "Enter new playlist name: " "New Playlist")
		      |0.5 0.5 0.5 0.9|)
  (tabs-index 'playlist-tabs (- (tabs-count 'playlist-tabs) 1)))

(defun on-stop ()
  (playback-stop)
  (playback-seek 0.0)
  (slider-value 'sl-seek 0.0)
  (component-enabled 'sl-seek nil))

(defun on-pause ()
  (playback-stop))

(defun on-play ()
  ;; TODO: fetch from playlist
  (component-enabled 'sl-seek t)
  (playback-start))

(defun toggle-playback ()
  (if (playback-is-playing)
      (on-pause)
    (on-play)))

(defun play-selected ()
  (setq current-id (playlist-get current-playlist current-index 'id))
  (playback-set-file (playlist-get current-playlist current-index 'path))
  (slider-range 'sl-seek 0.0 (playback-length)) ;; TODO: DRY
  (component-enabled 'sl-seek t) ;; TODO: playlist-select
  (playlist-select current-playlist current-index)
  (playback-start))

(defun on-next ()
  (if (< (+ 1 current-index) (playlist-items-count current-playlist))
      (progn (setq current-index (+ 1 current-index))
	     (play-selected))))

(defun on-prev ()
  (if (>= (- current-index 1) 0)
      (progn (setq current-index (- current-index 1))
	     (play-selected))))

(defun on-rand ()
  (progn (setq current-index (rand (playlist-items-count current-playlist)))
	 (play-selected)))

(defun on-playlist-click (item-str item-id item-index playlist-name)
  (playback-set-file item-str)
  (setq current-id item-id)
  (setq current-index item-index)
  (setq current-playlist playlist-name)
  (slider-range 'sl-seek 0.0 (playback-length))
  (component-enabled 'sl-seek t)
  (playback-start))

(defun playback-changed ()
  (if (playback-is-playing)
      (start-timer 'update-slider 100)
    (stop-timer 'update-slider)
    (if (playback-finished)
	(progn
	  (defvar current-value (ctags-get current-id 0))
	  (defvar first-play (ctags-get current-id 2))
	  (if current-value
		(ctags-set current-id 0 (+ 1 (ctags-get current-id 0))) ;; increase value
	    (ctags-set current-id 0 1)) ;; init value
	  (ctags-set current-id 3 (current-date)) ;; set timestamp
	  (if (not first-play)
	      (ctags-set current-id 2 (current-date))) ;; set first play date
	  ;;(message-box "playback-changed callback" "playback finished")
	  ;;(message-box "Current info" (strs current-playlist " : " current-index))
	  (on-next)
	  (repaint-component current-playlist)))))

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
(bind-mouse-click 'b-prev 'on-prev)
(bind-mouse-click 'b-next 'on-next)
(bind-mouse-click 'b-rand 'on-rand)
(bind-mouse-click 'b-new-playlist 'on-new-playlist)
(bind-slider-drag-end 'sl-seek 'on-slider-up '(slider-value))
(bind-slider-changed 'sl-gain 'on-slider-gain '(slider-value))
(create-timer 'update-slider 'on-update-slider)
(bind-playback 'playback-changed)

;; initialize ctags
(ctags-load "playback-stats")
(settings-load "settings")

;; load playlist from settings
(setq current-id (or (settings-get "current-index") 0))
(setq current-playlist (or (settings-get "current-playlist") nil))
(dolist e (or (settings-get "playlist-tabs") '())
	(message-box "playlist element: " (strs (nth 0 e) " / " (nth 1 e)))
	(tabs-add-component 'playlist-tabs
			    (init-playlist (nth 0 e))
			    (nth 1 e) |0.5 0.5 0.5 0.9|)
	(playlist-load (nth 0 e) (strs "playlists/" (nth 0 e))))

;; init audio settings
(audio-settings)

;; setup exit callback
(defun on-exit ()
  (defvar tabs (tabs-get-components 'playlist-tabs 'playlist))
  (dolist e tabs
	  (playlist-save (nth 0 e) (strs "playlists/" (nth 0 e))))
  (settings-set "playlist-tabs" tabs)
  (settings-set "current-index" current-index)
  (settings-set "current-playlist" current-playlist)
  (ctags-save "playback-stats")
  (settings-save "settings")
  (message-box "Exiting" "just exiting"))
(bind-exit 'on-exit)

;; bind hotkeys
(bind-hotkey "Pause" "" 'toggle-playback)

;; ;; filtered playlist test
;; (create-filtered-playlist 'playlist_0 'ppp
;; 			  (input-box "Search for:" "Enter query: " ""))
;; (playlist-add-column 'ppp "track" 'track 50 20 1000)
;; (playlist-add-column 'ppp "album" 'album 200 150 1000)
;; (playlist-add-column 'ppp "artist" 'artist 200 150 1000)
;; (playlist-add-column 'ppp "title" 'title 200 150 1000)
;; (playlist-add-column 'ppp "year" 'year 70 50 1000)
;; (playlist-add-column 'ppp "count" 'aaaaaaaaaaaaa 50 20 1000)
;; (create-window 'search-window "Search Results" |100.0 100.0 400.0 400.0| |1.0 1.0 1.0 1.0|)
;; (window-set-main-component 'search-window 'ppp)

;; TODO: replace current-playlist
(defun on-f3 ()
  (if (playlist-filter-enabled current-playlist)
      (playlist-disable-filter current-playlist)
    (playlist-enable-filter current-playlist (input-box "Quick search" "Query: " ""))))

(defun on-n ()
  (if (playlist-filter-enabled current-playlist)
      (playlist-filter-next current-playlist)))

(bind-key "F3" 'on-f3)
(bind-key "F4" 'on-n)

;; make things visible
(set-main-component 'l-main)
(refresh-interface)
